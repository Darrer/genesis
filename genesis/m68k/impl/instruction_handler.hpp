#ifndef __M68K_INSTRUCTION_HANDLER_HPP__
#define __M68K_INSTRUCTION_HANDLER_HPP__

#include "base_unit.h"
#include "ea_decoder.hpp"
#include "timings.hpp"

#include "exception.hpp"

#include <iostream>


namespace genesis::m68k
{

#define throw_invalid_opcode() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: invalid opcode")

class instruction_handler : public base_unit
{
private:
	enum inst_state : std::uint8_t
	{
		IDLE,
		STARTING,
		EXECUTING,
		DECODING,
	};

public:
	instruction_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq)
		: base_unit(regs, busm, pq), dec(busm, regs, pq)
	{
		reset();
	}

	void reset() override
	{
		state = IDLE;
		opcode = 0;
		exec_stage = 0;
		res = 0;

		base_unit::reset();
	}

	bool is_idle() const override
	{
		return state == IDLE && base_unit::is_idle();
	}

protected:
	void on_cycle() override
	{
		switch (state)
		{
		case IDLE:
			state = STARTING;
			[[fallthrough]];

		case STARTING:
			reset();
			opcode = pq.IRD;
			regs.PC += 2;
			state = EXECUTING;
			execute();
			break;

		case DECODING:
			if(dec.ready())
			{
				state = EXECUTING;
				execute();
			}
			break;

		case EXECUTING:
			execute();
			break;

		default:
			throw internal_error();
		}

		dec.cycle();
	}

	void set_idle() override
	{
		state = IDLE;
	}

private:
	void execute()
	{
		if((opcode >> 12) == 0b1101)
			exec_add();
		else if((opcode >> 8) == 0b110)
			exec_addi();
		else if((opcode >> 12) == 0b0101 && ((opcode >> 8) & 1) == 0)
			exec_addq();
		else
			throw not_implemented(std::to_string(opcode));
	}

	void exec_add()
	{
		// std::cout << "exec_add" << std::endl;

		const std::uint8_t size = dec_size(opcode >> 6);

		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			auto& reg = regs.D((opcode >> 9) & 0x7);
			auto op = dec.result();

			res = add(reg, op, size);

			const std::uint8_t opmode = (opcode >> 6) & 0x7;

			wait_after_idle(timings::add(opmode, op));

			// std::cout << "opmode: " << su::bin_str(opmode) << std::endl;
			if(opmode == 0b000 || opmode == 0b001 || opmode == 0b010)
			{
				store(reg, size, res);
				prefetch_one_and_idle();
			}
			else
			{
				prefetch_one();
			}

			break;
		}

		case 2:
			write_and_idle(dec.result().pointer().address, res, size);
			break;

		default: throw internal_error();
		}
	}

	void exec_addi()
	{
		// std::cout << "exec_addi" << std::endl;

		const std::uint8_t size = dec_size(opcode >> 6);

		switch (exec_stage++)
		{
		case 0:
			read_imm(size);
			break;

		case 1:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 2:
		{
			auto op = dec.result();

			res = add(imm, op, size);

			wait_after_idle(timings::addi(size, op));

			if(op.is_pointer())
			{
				prefetch_one();
			}
			else
			{
				store(op, size, res);
				prefetch_one_and_idle();
			}

			break;
		}

		case 3:
			write_and_idle(dec.result().pointer().address, res, size);
			break;

		default: throw internal_error();
		}
	}

	void exec_addq()
	{
		const std::uint8_t size = dec_size(opcode >> 6);

		// std::cout << "exec_addq" << std::endl;

		switch (exec_stage++)
		{
		case 0:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			std::uint8_t data = (opcode >> 9) & 0x7;
			if(data == 0) data = 8;

			auto op = dec.result();

			res = add(data, op, size);

			wait_after_idle(timings::addq(size, op));

			if(op.is_pointer())
			{
				prefetch_one();
			}
			else
			{
				store(op, size, res);
				prefetch_one_and_idle();
			}

			break;
		}

		case 2:
			write_and_idle(dec.result().pointer().address, res, size);
			break;

		default: throw internal_error();
		}
	}

	std::uint8_t dec_size(std::uint8_t size)
	{
		size = size & 0b11;
		if(size == 0) return 1;
		if(size == 1) return 2;
		if(size == 2) return 4;

		throw_invalid_opcode();
	}

private:
	std::uint32_t reg_value(data_register& reg, std::uint8_t size)
	{
		if(size == 1)
			return reg.B;
		else if(size == 2)
			return reg.W;
		else
			return reg.LW;
	}

	std::uint32_t op_value(operand& op, std::uint8_t size)
	{
		if(size == 1)
		{
			if(op.is_data_reg())
				return op.data_reg().B;
			else if(op.is_addr_reg())
				throw_invalid_opcode();
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}
		else if(size == 2)
		{
			if(op.is_data_reg())
				return op.data_reg().W;
			else if(op.is_addr_reg())
				return op.addr_reg().W;
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}
		else if(size == 4)
		{
			if(op.is_data_reg())
				return op.data_reg().LW;
			else if(op.is_addr_reg())
				return op.addr_reg().LW;
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}

		throw internal_error();
	}

	void store(data_register& d, std::uint8_t size, std::uint32_t res)
	{
		if(size == 1)
		{
			d.B = (std::uint8_t)res;
		}
		else if(size == 2)
		{
			d.W = (std::uint16_t)res;
		}
		else if(size == 4)
		{
			d.LW = res;
		}
		else
		{
			throw internal_error();
		}
	}

	void store(operand& op, std::uint8_t size, std::uint32_t res)
	{
		if(size == 1)
		{
			if(op.is_data_reg())
				op.data_reg().B = (std::uint8_t)res;
			else
				throw_invalid_opcode();
		}
		else if(size == 2)
		{
			if(op.is_data_reg())
				op.data_reg().W = (std::uint16_t)res;
			else if(op.is_addr_reg())
				op.addr_reg().W = (std::uint16_t)res;
		}
		else if(size == 4)
		{
			if(op.is_data_reg())
				op.data_reg().LW = res;
			else if(op.is_addr_reg())
				op.addr_reg().LW = res;
		}

		// throw internal_error();
	}

	std::uint32_t add(data_register& reg, operand& op, std::uint8_t size)
	{
		return reg_value(reg, size) + op_value(op, size);
	}

	std::uint32_t add(std::uint32_t val, operand& op, std::uint8_t size)
	{
		return val + op_value(op, size);
	}

private:
	void decode_ea(std::uint8_t ea, std::uint8_t size)
	{
		dec.decode(ea, size);
		if(dec.ready())
		{
			// immediate decoding
			execute();
		}
		else
		{
			// wait till decoding is over
			state = DECODING;
		}
	}

private:
	m68k::ea_decoder dec;

	std::uint16_t opcode = 0;
	std::uint32_t res = 0;
	std::uint8_t exec_stage;

	std::uint8_t state;
};

}

#endif // __M68K_INSTRUCTION_HANDLER_HPP__
