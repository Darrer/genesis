#ifndef __M68K_INSTRUCTION_UNIT_HPP__
#define __M68K_INSTRUCTION_UNIT_HPP__


#include "base_unit.h"
#include "instruction_type.h"
#include "ea_decoder.hpp"
#include "timings.hpp"
#include "operations.hpp"

#include "opcode_decoder.h"

#include "exception.hpp"

#include <iostream>


namespace genesis::m68k
{

#define throw_invalid_opcode() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: invalid opcode")

class instruction_unit : public base_unit
{
private:
	enum inst_state : std::uint8_t
	{
		IDLE,
		EXECUTING,
		DECODING,
		WRITE_RESULT,
	};

public:
	instruction_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq)
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
			curr_inst = decode_opcode(opcode);
			// std::cout << "Executing: " << (int)curr_inst << std::endl;
			regs.PC += 2;
			regs.INST_PC = regs.PC;
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

		case WRITE_RESULT:
			write_and_idle(addr, res, size);
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
		switch (curr_inst)
		{
		case inst_type::ADD:
		case inst_type::SUB:
		case inst_type::AND:
		case inst_type::OR:
		case inst_type::EOR:
		case inst_type::CMP:
			alu_mode_handler();
			break;

		case inst_type::ADDA:
		case inst_type::SUBA:
		case inst_type::CMPA:
			alu_address_mode_handler();
			break;

		case inst_type::ADDI:
		case inst_type::ANDI:
		case inst_type::SUBI:
		case inst_type::ORI:
		case inst_type::EORI:
		case inst_type::CMPI:
			alu_imm_handler();
			break;

		case inst_type::ADDQ:
		case inst_type::SUBQ:
			alu_quick_handler();
			break;

		case inst_type::CMPM:
			rm_postinc_handler();
			break;

		case inst_type::NEG:
		case inst_type::NEGX:
		case inst_type::NOT:
			unary_handler();
			break;

		case inst_type::ADDX:
		case inst_type::SUBX:
			rm_predec_handler();
			break;

		case inst_type::NOP:
			nop_hanlder();
			break;

		case inst_type::MOVE:
			move_handler();
			break;

		default: throw internal_error();
		}
	}

	// TODO: in most cases we do not handle invalid opcodes properly
	// especially with some N/A EA decoding

	void alu_mode_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			auto& reg = regs.D((opcode >> 9) & 0x7);
			auto op = dec.result();

			const std::uint8_t opmode = (opcode >> 6) & 0x7;

			wait_after_idle(timings::alu_mode(curr_inst, opmode, op));

			// std::cout << "opmode: " << su::bin_str(opmode) << std::endl;
			if(opmode == 0b000 || opmode == 0b001 || opmode == 0b010)
			{
				res = operations::alu(curr_inst, reg, op, size, regs.flags);
				if(curr_inst != inst_type::CMP)
					store(reg, size, res);
				prefetch_one_and_idle();
			}
			else
			{
				res = operations::alu(curr_inst, op, reg, size, regs.flags);
				save_prefetch_and_idle(op, res, size);
			}

			break;
		}

		default: throw internal_error();
		}
	}

	void alu_address_mode_handler()
	{
		const std::uint8_t opmode = (opcode >> 6) & 0x7;
		if(opmode != 0b011 && opmode != 0b111)
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			size = opmode == 0b011 ? 2 : 4;
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			auto& reg = regs.A((opcode >> 9) & 0x7);
			auto op = dec.result();

			wait_after_idle(timings::alu_mode(curr_inst, opmode, op));
			reg.LW = operations::alu(curr_inst, op, reg.LW, size, regs.flags);
			prefetch_one_and_idle();

			break;
		}

		default: throw internal_error();
		}
	}

	void alu_imm_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			read_imm(size);
			break;

		case 1:
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 2:
		{
			auto op = dec.result();

			res = operations::alu(curr_inst, op, imm, size, regs.flags);

			wait_after_idle(timings::alu_size(curr_inst, size, op));

			if(curr_inst == inst_type::CMPI)
			{
				prefetch_one_and_idle();
				break;
			}

			save_prefetch_and_idle(op, res, size);
			break;
		}

		default: throw internal_error();
		}
	}

	void alu_quick_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			std::uint8_t data = (opcode >> 9) & 0x7;
			if(data == 0) data = 8;

			auto op = dec.result();

			res = operations::alu(curr_inst, op, data, size, flags);
			if(!op.is_addr_reg())
				update_user_bits(flags);

			wait_after_idle(timings::alu_size(curr_inst, size, op));

			save_prefetch_and_idle(op, res, size);
			break;
		}

		default: throw internal_error();
		}
	}

	void rm_postinc_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			src_reg = opcode & 0x7;
			dest_reg = (opcode >> 9) & 0x7;
			size = dec_size(opcode >> 6);

			if((opcode >> 3) & 1)
			{
				// address register
				read(regs.A(src_reg).LW, size);
				inc_addr(src_reg, size);
			}
			else
			{
				// data register
				auto src = regs.D(src_reg);
				auto dest = regs.D(dest_reg);
				res = operations::alu(curr_inst, data, res, size, regs.flags);
				store(dest, size, res);
				prefetch_one_and_idle();
			}

			break;


		case 1:
			res = data;
			read(regs.A(dest_reg).LW, size);
			inc_addr(dest_reg, size);
			break;

		case 2:
			operations::alu(curr_inst, data, res, size, regs.flags);
			prefetch_one_and_idle();
			break;

		default: throw internal_error();
		}
	}

	void rm_predec_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			src_reg = opcode & 0x7;
			dest_reg = (opcode >> 9) & 0x7;
			size = dec_size(opcode >> 6);

			if((opcode >> 3) & 1)
			{
				// address register
				wait(2);
			}
			else
			{
				// data register
				auto& src = regs.D(src_reg);
				auto& dest = regs.D(dest_reg);
				res = operations::alu(curr_inst, dest, src, size, regs.flags);
				if(size == 4) wait_after_idle(4);
				store(dest, size, res);
				prefetch_one_and_idle();
			}

			break;

		/* Got here if it's an address register */
		case 1:
			dec_and_read(src_reg, size);
			break;

		case 2:
			res = data;
			dec_and_read(dest_reg, size);
			break;

		case 3:
			res = operations::alu(curr_inst, data, res, size, regs.flags);
			if(size == 4)
			{
				// in this particular case we need to:
				// 1. write LSW
				// 2. do prefetch
				// 3. write MSW
				write_word(regs.A(dest_reg).LW + 2, res & 0xFFFF);
			}
			else
			{
				prefetch_one();
			}
			break;

		case 4:
			if(size == 4)
			{
				prefetch_one();
			}
			else
			{
				write_and_idle(regs.A(dest_reg).LW, res, size);
			}
			break;
		
		case 5:
			write_word_and_idle(regs.A(dest_reg).LW, res >> 16);
			break;

		default: throw internal_error();
		}
	}

	void unary_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			break;

		case 1:
		{
			auto op = dec.result();

			res = operations::alu(curr_inst, op, size, regs.flags);

			wait_after_idle(timings::alu_size(curr_inst, size, op));
			save_prefetch_and_idle(op, res, size);

			break;
		}

		default: throw internal_error();
		}
	}

	void nop_hanlder()
	{
		if(busm.is_idle())
		{
			prefetch_one_and_idle();
		}
	}

	void move_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			// decode source
			size = dec_move_size(opcode >> 12);
			decode_ea(opcode & 0xFF, size);
			break;

		case 1:
		{
			src_op = dec.result();
			// decode destenation
			std::uint8_t ea = (opcode >> 6) & 7;
			ea = (ea << 3) | ((opcode >> 9) & 7);
			decode_ea(ea, size);
			break;
		}

		case 2:
		{
			auto dest_op = dec.result();

			res = operations::alu(curr_inst, src_op.value(), dest_op, size, regs.flags);

			if(((opcode >> 6) & 7) == 0b100)
			{
				if(dest_op.is_pointer())
				{
					addr = dest_op.pointer().address;
					prefetch_one();
				}
				else
				{
					throw internal_error();
				}
			}
			else
			{
				if(dest_op.is_pointer())
				{
					write(dest_op.pointer().address, res, size);
				}
				else
				{
					store(dest_op, size, res);
					prefetch_one_and_idle();
				}
			}

			break;
		}

		case 3:
			if(((opcode >> 6) & 7) == 0b100)
			{
				write_and_idle(addr, res, size);
			}
			else
			{
				prefetch_one_and_idle();
			}
			break;

		default: throw internal_error();
		}
	}

	// save the result (write to register or to memory), do prefetch and go IDLE
	void save_prefetch_and_idle(operand& op, std::uint32_t res, std::uint8_t size)
	{
		if(!op.is_pointer())
		{
			store(op, size, res);
			prefetch_one_and_idle();
		}
		else
		{
			prefetch_one();

			// schedule write 
			state = WRITE_RESULT;
			this->res = res;
			this->size = size;
			addr = op.pointer().address;
		}
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

	void store(address_register& a, std::uint32_t res)
	{
		a.LW = res;
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

	std::uint8_t dec_size(std::uint8_t size)
	{
		size = size & 0b11;
		if(size == 0) return 1;
		if(size == 1) return 2;
		if(size == 2) return 4;

		throw_invalid_opcode();
	}

	std::uint8_t dec_move_size(std::uint8_t size)
	{
		size = size & 0b11;
		if(size == 0b01) return 1;
		if(size == 0b11) return 2;
		if(size == 0b10) return 4;

		throw_invalid_opcode();
	}

	inst_type decode_opcode(std::uint16_t opcode)
	{
		auto res = opcode_decoder::decode(opcode);
		if(res == inst_type::NONE)
			throw not_implemented();
		return res;
	}

private:
	m68k::ea_decoder dec;

	std::uint16_t opcode = 0;
	inst_type curr_inst;
	std::uint8_t exec_stage;

	std::uint8_t state;

	// some helper variables
	std::uint32_t res = 0;
	std::uint32_t addr = 0;
	std::uint8_t size = 0;
	std::uint8_t src_reg = 0;
	std::uint8_t dest_reg = 0;
	std::optional<m68k::operand> src_op;
	status_register flags;
};

}

#endif // __M68K_INSTRUCTION_UNIT_HPP__
