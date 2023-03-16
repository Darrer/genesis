#ifndef __M68K_INSTRUCTION_HANDLER_HPP__
#define __M68K_INSTRUCTION_HANDLER_HPP__

#include "base_unit.h"
#include "ea_decoder.hpp"
#include "timings.hpp"
#include "operations.hpp"

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
		EXECUTING,
		DECODING,
	};

public:
	instruction_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq)
		: base_unit(regs, busm, pq), dec(busm, regs, pq)
	{
		reset();
	}

	// rename to on_idle
	void reset() override
	{
		state = IDLE;
		exec_stage = 0;
		res = 0;
		dec.reset();

		base_unit::reset();
	}

protected:
	void on_cycle() override
	{
		switch (state)
		{
		case IDLE:
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

		if(state == DECODING)
			dec.cycle();
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

			res = operations::add(reg, op, size, sr);
			update_user_bits(sr);

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

			res = operations::add(imm, op, size, sr);
			update_user_bits(sr);

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

			res = operations::add(data, op, size, sr);
			if(!op.is_addr_reg())
				update_user_bits(sr);

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
		else
		{
			d.LW = res;
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
	}

	void update_user_bits(status_register sr)
	{
		auto& f = regs.flags;
		f.C = sr.C;
		f.V = sr.V;
		f.Z = sr.Z;
		f.N = sr.N;
		f.X = sr.X;
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
	status_register sr;
	std::uint8_t exec_stage;

	std::uint8_t state;
};

}

#endif // __M68K_INSTRUCTION_HANDLER_HPP__
