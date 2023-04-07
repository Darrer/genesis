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

// TODO: add exception class
#define throw_invalid_opcode() \
	throw std::runtime_error(std::string("executioner::") + __func__ + " error: invalid opcode")

class instruction_unit : public base_unit
{
private:
	enum inst_state : std::uint8_t
	{
		IDLE,
		EXECUTING,
	};

public:
	instruction_unit(m68k::cpu_registers& regs, m68k::bus_scheduler& scheduler)
		: base_unit(regs, scheduler), dec(regs, scheduler), move_dec(regs, scheduler)
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
	exec_state on_executing() override
	{
		switch (state)
		{
		case IDLE:
			opcode = regs.IRD;
			regs.SIRD = regs.IRD;
			curr_inst = decode_opcode(opcode);
			// std::cout << "Executing: " << (int)curr_inst << std::endl;
			regs.PC += 2;
			state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			return execute();

		default:
			throw internal_error();
		}
	}

private:
	exec_state execute()
	{
		switch (curr_inst)
		{
		case inst_type::ADD:
		case inst_type::SUB:
		case inst_type::AND:
		case inst_type::OR:
		case inst_type::EOR:
		case inst_type::CMP:
			return alu_mode_handler();

		case inst_type::ADDA:
		case inst_type::SUBA:
		case inst_type::CMPA:
			return alu_address_mode_handler();

		case inst_type::ADDI:
		case inst_type::ANDI:
		case inst_type::SUBI:
		case inst_type::ORI:
		case inst_type::EORI:
		case inst_type::CMPI:
			return alu_imm_handler();

		case inst_type::ADDQ:
		case inst_type::SUBQ:
			return alu_quick_handler();

		case inst_type::CMPM:
			return rm_postinc_handler();

		case inst_type::NEG:
		case inst_type::NEGX:
		case inst_type::NOT:
			return unary_handler();

		case inst_type::ADDX:
		case inst_type::SUBX:
			return rm_predec_handler();

		case inst_type::NOP:
			return nop_hanlder();

		case inst_type::MOVE:
			return move_handler();

		case inst_type::MOVEQ:
			return moveq_handler();

		default: throw internal_error();
		}
	}

	// TODO: in most cases we do not handle invalid opcodes properly
	// especially with some N/A EA decoding

	exec_state alu_mode_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto& reg = regs.D((opcode >> 9) & 0x7);
			auto op = dec.result();

			const std::uint8_t opmode = (opcode >> 6) & 0x7;
			if(opmode == 0b000 || opmode == 0b001 || opmode == 0b010)
			{
				res = operations::alu(curr_inst, reg, op, size, regs.flags);
				if(curr_inst != inst_type::CMP)
					store(reg, size, res);
				scheduler.prefetch_one();
			}
			else
			{
				res = operations::alu(curr_inst, op, reg, size, regs.flags);
				schedule_prefetch_and_write(op, res, size);
			}

			scheduler.wait(timings::alu_mode(curr_inst, opmode, op));
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state alu_address_mode_handler()
	{
		const std::uint8_t opmode = (opcode >> 6) & 0x7;
		if(opmode != 0b011 && opmode != 0b111)
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			size = opmode == 0b011 ? 2 : 4;
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto& reg = regs.A((opcode >> 9) & 0x7);
			auto op = dec.result();

			reg.LW = operations::alu(curr_inst, op, reg.LW, size, regs.flags);

			scheduler.prefetch_one();
			scheduler.wait(timings::alu_mode(curr_inst, opmode, op));
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state alu_imm_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			read_imm(size);
			return exec_state::wait_scheduler;

		case 1:
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 2:
		{
			auto op = dec.result();

			res = operations::alu(curr_inst, op, imm, size, regs.flags);

			if(curr_inst == inst_type::CMPI)
			{
				scheduler.prefetch_one();
			}
			else
			{
				schedule_prefetch_and_write(op, res, size);
			}

			scheduler.wait(timings::alu_size(curr_inst, size, op));
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state alu_quick_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
		{
			std::uint8_t data = (opcode >> 9) & 0x7;
			if(data == 0) data = 8;

			auto op = dec.result();

			res = operations::alu(curr_inst, op, data, size, flags);
			if(!op.is_addr_reg())
				update_user_bits(flags);

			schedule_prefetch_and_write(op, res, size);

			scheduler.wait(timings::alu_size(curr_inst, size, op));
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state rm_postinc_handler()
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
				// FIXME: read may rise an exception, register is not inc in such a case
				regs.inc_addr(src_reg, size);
				return exec_state::wait_scheduler;
			}
			else
			{
				// data register
				auto src = regs.D(src_reg);
				auto dest = regs.D(dest_reg);
				res = operations::alu(curr_inst, data, res, size, regs.flags);
				store(dest, size, res);
				scheduler.prefetch_one();
				return exec_state::done;
			}

		case 1:
			res = data;
			read(regs.A(dest_reg).LW, size);
			// FIXME: read may rise an exception, register is not inc in such a case
			regs.inc_addr(dest_reg, size);
			return exec_state::wait_scheduler;

		case 2:
			operations::alu(curr_inst, data, res, size, regs.flags);
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state rm_predec_handler()
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
				scheduler.wait(2);
				return exec_state::wait_scheduler;
			}
			else
			{
				// data register
				auto& src = regs.D(src_reg);
				auto& dest = regs.D(dest_reg);
				res = operations::alu(curr_inst, dest, src, size, regs.flags);
				store(dest, size, res);
				scheduler.prefetch_one();
				if(size == 4) scheduler.wait(4);
				return exec_state::done;
			}

		/* Got here if it's an address register */
		case 1:
			dec_and_read(src_reg, size);
			return exec_state::wait_scheduler;

		case 2:
			res = data;
			dec_and_read(dest_reg, size);
			return exec_state::wait_scheduler;

		case 3:
			res = operations::alu(curr_inst, data, res, size, regs.flags);
			if(size == 4)
			{
				// in this particular case we need to:
				// 1. write LSW
				// 2. do prefetch
				// 3. write MSW
				scheduler.write(regs.A(dest_reg).LW + 2, res & 0xFFFF, size_type::WORD);
				scheduler.prefetch_one();
				scheduler.write(regs.A(dest_reg).LW, res >> 16, size_type::WORD);
			}
			else
			{
				scheduler.prefetch_one();
				scheduler.write(regs.A(dest_reg).LW, res, (size_type)size);
			}
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state unary_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto op = dec.result();

			res = operations::alu(curr_inst, op, size, regs.flags);

			schedule_prefetch_and_write(op, res, size);
			scheduler.wait(timings::alu_size(curr_inst, size, op));

			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state nop_hanlder()
	{
		scheduler.prefetch_one();
		return exec_state::done;
	}

	exec_state move_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			// decode source
			size = dec_move_size(opcode >> 12);
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto src_op = dec.result();
			res = operations::alu(curr_inst, src_op, size, regs.flags);
			return move_decode_and_write(src_op, res, (size_type)size);
		}

		default: throw internal_error();
		}
	}

	exec_state move_decode_and_write(operand src_op, std::uint32_t res, size_type size)
	{
		std::uint8_t ea = opcode >> 6;
		std::uint8_t mode = ea & 0x7;
		std::uint8_t reg = (ea >> 3) & 0x7;
		dest_reg = reg;

		// std::cout << "decode move: " << (int)mode << ", " << (int)reg << std::endl;

		order write_order = order::msw_first;

		switch (mode)
		{
		case 0b000:
			store(regs.D(reg), size, res);
			scheduler.prefetch_one();
			return exec_state::done;
		
		case 0b010:
			scheduler.write(regs.A(reg).LW, res, (size_type)size, write_order);
			scheduler.prefetch_one();
			return exec_state::done;

		case 0b011:
			scheduler.write(regs.A(reg).LW, res, (size_type)size, write_order);
			scheduler.prefetch_one();
			scheduler.call([this]()
			{
				regs.inc_addr(dest_reg, this->size);
			});
			return exec_state::done;

		case 0b100:
			scheduler.prefetch_one();
			if(size != size_type::LONG)
			{
				regs.dec_addr(dest_reg, size);
				scheduler.write(regs.A(reg).LW, res, (size_type)size);
			}
			else
			{
				regs.dec_addr(dest_reg, size_type::WORD);
				scheduler.write(regs.A(reg).LW - 2, res, (size_type)size);
				scheduler.call([this]()
				{
					regs.dec_addr(dest_reg, size_type::WORD);
				});
			}

			return exec_state::done;

		case 0b101:
			scheduler.prefetch_irc();
			addr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)regs.IRC);
			scheduler.write(addr, res, (size_type)size, write_order);
			scheduler.prefetch_one();
			return exec_state::done;

		case 0b110:
			scheduler.wait(2);
			scheduler.prefetch_irc();
			addr = ea_decoder::dec_brief_reg(regs.A(reg).LW, regs.IRC, regs);
			scheduler.write(addr, res, (size_type)size, write_order);
			scheduler.prefetch_one();
			return exec_state::done;

		case 0b111:
		{
			switch (reg)
			{
			case 0b000:
				scheduler.prefetch_irc();
				scheduler.write((std::int16_t)regs.IRC, res, (size_type)size, write_order);
				scheduler.prefetch_one();
				return exec_state::done;

			case 0b001:
				addr = regs.IRC << 16;
				scheduler.prefetch_irc();
				if(src_op.is_pointer())
				{
					scheduler.call([this]()
					{
						addr = addr | (regs.IRC & 0xFFFF);
						scheduler.write(addr, this->res, (size_type)this->size, order::msw_first);
						scheduler.prefetch_irc();
						scheduler.prefetch_one();
					});
				}
				else
				{
					scheduler.call([this]()
					{
						addr = addr | (regs.IRC & 0xFFFF);
						scheduler.prefetch_irc();
						scheduler.write(addr, this->res, (size_type)this->size, order::msw_first);
						scheduler.prefetch_one();
					});
				}
				return exec_state::done;

			default: throw internal_error();
			}
		}

		default: throw internal_error();
		}
	}

	exec_state moveq_handler()
	{
		std::int32_t data = std::int8_t(opcode & 0xFF);
		std::uint8_t reg = (opcode >> 9) & 0x7;

		operations::move(data, size_type::LONG, regs.flags);
		store(regs.D(reg), size_type::LONG, data);
		scheduler.prefetch_one();

		return exec_state::done;
	}

	void decode_ea_move(std::uint8_t ea, size_type size)
	{
		std::uint8_t mode = ea & 0x7;
		std::uint8_t reg = (ea >> 3) & 0x7;
		dest_op.reset();

		std::cout << "decode ea move: " << (int)mode << ", " << (int)reg << std::endl;

		auto save = [this](std::uint32_t addr, size_type size)
		{
			dest_op = { operand::raw_pointer(addr), size };
		};

		switch (mode)
		{
		case 0b000:
			dest_op = { regs.D(reg), size };
			return;
		
		case 0b010:
			save(regs.A(reg).LW, size);
			return;

		case 0b011:
			save(regs.A(reg).LW, size);
			regs.inc_addr(reg, size);
			return;

		case 0b100:
			return;

		case 0b101:
		{
			scheduler.prefetch_irc();
			std::uint32_t ptr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)regs.IRC);
			save(ptr, size);
			return;
		}

		case 0b110:
			return;
		
		case 0b111:
		{
		switch (reg)
		{
		case 0b000:
			return;
		
		case 0b001:
			return;
		
		default: throw internal_error();
		}

		}

		default: throw internal_error();
		}
	}

	// save the result (write to register or to memory), do prefetch and go IDLE
	void schedule_prefetch_and_write(operand& op, std::uint32_t res, std::uint8_t size)
	{
		scheduler.prefetch_one();
		if(!op.is_pointer())
		{
			store(op, size, res);
		}
		else
		{
			scheduler.write(op.pointer().address, res, (size_type)size);
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
	m68k::ea_move_decoder move_dec;

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
	std::optional<m68k::operand> dest_op;
	status_register flags;
};

}

#endif // __M68K_INSTRUCTION_UNIT_HPP__
