#ifndef __M68K_INSTRUCTION_UNIT_HPP__
#define __M68K_INSTRUCTION_UNIT_HPP__


#include "base_unit.h"
#include "cpu_registers.hpp"
#include "bus_scheduler.h"
#include "exception_manager.h"


#include "instruction_type.h"
#include "opcode_decoder.h"
#include "ea_decoder.hpp"
#include "timings.hpp"
#include "operations.hpp"

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
	instruction_unit(m68k::cpu_registers& regs, exception_manager& exman, m68k::bus_scheduler& scheduler)
		: base_unit(regs, scheduler), dec(regs, scheduler), exman(exman)
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
			regs.SPC = regs.PC;
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
		case inst_type::CLR:
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

		case inst_type::MOVEA:
			return movea_handler();

		case inst_type::MOVEM:
			return movem_handler();

		case inst_type::MOVEP:
			return movep_handler();
		
		case inst_type::MOVEfromSR:
			return move_from_sr_handler();
		
		case inst_type::MOVEtoSR:
			return move_to_sr_handler();

		case inst_type::MOVE_USP:
			return move_usp_handler();
		
		case inst_type::MOVEtoCCR:
			return move_to_ccr_handler();

		case inst_type::ANDItoCCR:
		case inst_type::ORItoCCR:
		case inst_type::EORItoCCR:
			return alu_to_ccr_handler();
		
		case inst_type::ANDItoSR:
		case inst_type::ORItoSR:
		case inst_type::EORItoSR:
			return alu_to_sr_handler();
		
		case inst_type::ASLRreg:
		case inst_type::ROLRreg:
		case inst_type::LSLRreg:
		case inst_type::ROXLRreg:
			return shift_reg_handler();

		case inst_type::ASLRmem:
		case inst_type::ROLRmem:
		case inst_type::LSLRmem:
		case inst_type::ROXLRmem:
			return shift_mem_handler();

		case inst_type::TST:
			return tst_handler();

		case inst_type::MULU:
		case inst_type::MULS:
			return mulu_handler();

		case inst_type::TRAP:
			return trap_handler();

		case inst_type::TRAPV:
			return trapv_handler();

		case inst_type::DIVU:
		case inst_type::DIVS:
			return div_handler();

		case inst_type::EXT:
			return ext_handler();

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
			size = opmode == 0b011 ? size_type::WORD : size_type::LONG;
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
				if(size == size_type::LONG) scheduler.wait(4);
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
			if(size == size_type::LONG)
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
				scheduler.write(regs.A(dest_reg).LW, res, size);
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
			return decode_move_and_write(src_op, res, size);
		}

		default: throw internal_error();
		}
	}

	exec_state decode_move_and_write(operand src_op, std::uint32_t res, size_type size)
	{
		std::uint8_t ea = opcode >> 6;
		std::uint8_t mode = ea & 0x7;
		dest_reg = (ea >> 3) & 0x7;

		if(mode == 0b000 || mode == 0b010 || mode == 0b101 || mode == 0b110 || (mode == 0b111 && dest_reg == 0b000))
		{
			std::uint8_t ea_move = (mode << 3) | dest_reg;
			dec.schedule_decoding(ea_move, size, ea_decoder::flags::no_read);

			scheduler.call([this]()
			{
				auto op = dec.result();

				if(op.is_pointer())
				{
					std::uint32_t addr = op.pointer().address;
					scheduler.write(addr, this->res, this->size, order::msw_first);
				}
				else
				{
					store(op, this->size, this->res);
				}
				scheduler.prefetch_one();
			});

			return exec_state::done;
		}

		switch (mode)
		{
		// case 0b000:
		// 	store(regs.D(dest_reg), size, res);
		// 	scheduler.prefetch_one();
		// 	return exec_state::done;
		
		// case 0b010:
		// 	scheduler.write(regs.A(dest_reg).LW, res, size, order::msw_first);
		// 	scheduler.prefetch_one();
		// 	return exec_state::done;

		case 0b011:
			scheduler.write(regs.A(dest_reg).LW, res, size, order::msw_first);
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
				scheduler.write(regs.A(dest_reg).LW, res, size);
			}
			else
			{
				regs.dec_addr(dest_reg, size_type::WORD);
				scheduler.write(regs.A(dest_reg).LW - 2, res, size);
				scheduler.call([this]()
				{
					regs.dec_addr(dest_reg, size_type::WORD);
				});
			}

			return exec_state::done;

		// case 0b101:
		// 	scheduler.prefetch_irc();
		// 	addr = (std::int32_t)regs.A(dest_reg).LW + std::int32_t((std::int16_t)regs.IRC);
		// 	scheduler.write(addr, res, size, order::msw_first);
		// 	scheduler.prefetch_one();
		// 	return exec_state::done;

		// case 0b110:
		// 	scheduler.wait(2);
		// 	scheduler.prefetch_irc();
		// 	addr = ea_decoder::dec_brief_reg(regs.A(dest_reg).LW, regs.IRC, regs);
		// 	scheduler.write(addr, res, size, order::msw_first);
		// 	scheduler.prefetch_one();
		// 	return exec_state::done;

		case 0b111:
		{
			switch (dest_reg)
			{
			// case 0b000:
			// 	scheduler.prefetch_irc();
			// 	scheduler.write((std::int16_t)regs.IRC, res, size, order::msw_first);
			// 	scheduler.prefetch_one();
			// 	return exec_state::done;

			case 0b001:
				addr = regs.IRC << 16;
				scheduler.prefetch_irc();
				if(src_op.is_pointer())
				{
					scheduler.call([this]()
					{
						addr = addr | (regs.IRC & 0xFFFF);
						scheduler.write(addr, this->res, this->size, order::msw_first);
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
						scheduler.write(addr, this->res, this->size, order::msw_first);
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

	exec_state movea_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = ((opcode >> 12) & 1) == 0 ? size_type::LONG : size_type::WORD;
			dest_reg = (opcode >> 9) & 0x7;
			dec.schedule_decoding(opcode & 0xFF, size);
			return exec_state::wait_scheduler;

		case 1:
			res = operations::movea(dec.result(), size);
			store(regs.A(dest_reg), res);
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state movem_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = ((opcode >> 6) & 1) == 0 ? size_type::WORD : size_type::LONG;
			read_imm(size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
			src_reg = opcode & 0x7;
			dec.schedule_decoding(opcode & 0xFF, size, ea_decoder::flags::no_read);
			return exec_state::wait_scheduler;

		case 2:
		{
			if(!dec.result().is_pointer())
				throw_invalid_opcode();

			std::uint8_t dr = (opcode >> 10) & 1;
			std::uint16_t reg_mask = imm & 0xFFFF;

			if(dr == 1) 
				movem_memory_to_register(reg_mask);
			else
				movem_register_to_memory(reg_mask);

			scheduler.prefetch_one();
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	void movem_register_to_memory(std::uint16_t reg_mask)
	{
		std::uint32_t start_addr = dec.result().pointer().address;
		std::int8_t offset = size == size_type::WORD ? 2 : 4;
		order order = order::msw_first;

		bool predec_mode = ((opcode >> 3) & 0x7) == 0b100;
		if(predec_mode)
		{
			order = order::lsw_first;
			offset = -offset;
			start_addr += offset;
		}

		for(int i = 0; i <= 15; ++i)
		{
			if(((reg_mask >> i) & 1) == 0)
				continue;

			std::uint8_t reg = predec_mode ? (15 - i) : i;
			std::uint32_t data;
			if(reg >= 8)
			{
				// address register
				data = regs.A(reg - 8).LW;
			}
			else
			{
				// data register
				data = regs.D(reg).LW;
			}

			scheduler.write(start_addr, data, size, order);
			start_addr += offset;
		}

		if(predec_mode)
		{
			addr = predec_mode ? (start_addr - offset) : start_addr;
			scheduler.call([this]()
			{
				regs.A(src_reg).LW = addr;
			});
		}
	}

	void move_to_register(std::uint32_t data, size_type size)
	{
		if(size == size_type::WORD)
			data = std::int32_t(std::int16_t(data & 0xFFFF));

		for(; dest_reg <= 15; ++dest_reg)
		{
			if(((move_reg_mask >> dest_reg) & 1) == 0)
				continue;

			if(dest_reg >= 8)
			{
				// address register
				auto& reg = regs.A(dest_reg - 8);
				reg.LW = data;
			}
			else
			{
				// data register
				auto& reg = regs.D(dest_reg);
				reg.LW = data;
			}

			++dest_reg;
			return;
		}

		// must be unrechable
		throw internal_error();
	}

	void movem_memory_to_register(std::uint16_t reg_mask)
	{
		move_reg_mask = reg_mask;
		dest_reg = 0;
		bool postinc_mode = ((opcode >> 3) & 0x7) == 0b011;
		if(postinc_mode)
		{
			// in the end of execution we update reg with address value,
			// we don't need to inc it, however, there might be an exception,
			// so inc it to accout for that.
			if(size == size_type::LONG)
				regs.inc_addr(src_reg, size_type::WORD);
			else
				regs.inc_addr(src_reg, size);
		}

		std::uint32_t start_addr = dec.result().pointer().address;
		for(int i = 0; i <= 15; ++i)
		{
			if(((reg_mask >> i) & 1) == 0)
				continue;

			scheduler.read(start_addr, size, [this](std::uint32_t data, size_type size)
			{
				move_to_register(data, size);
			});

			if(size == size_type::WORD)
				start_addr += 2;
			else
				start_addr += 4;
		}

		// some weird uncodumented read
		scheduler.read(start_addr, size_type::WORD, nullptr);

		if(postinc_mode)
		{
			addr = start_addr;
			scheduler.call([this]()
			{
				regs.A(src_reg).LW = addr;
			});
		}
	}

	exec_state movep_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = ((opcode >> 6) & 1) == 0 ? size_type::WORD : size_type::LONG;
			dest_reg = (opcode >> 9) & 0x7;
			src_reg = opcode & 0x7;

			read_imm(size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
		{
			addr = regs.A(src_reg).LW + operations::sign_extend(imm & 0xFFFF);
			bool mem_to_reg = ((opcode >> 7) & 1) == 0;
			if(mem_to_reg)
				movep_memory_to_register();
			else
				movep_register_to_memory();
			
			scheduler.prefetch_one();
			return exec_state::done;
		}
		
		default: throw internal_error();
		}
	}

	void movep_memory_to_register()
	{
		auto on_read = [this](std::uint32_t data, size_type)
		{
			auto& reg = regs.D(dest_reg);
			if(size == size_type::LONG)
				reg.LW = (reg.LW << 8) | (data & 0xFF);
			else
				reg.W = (reg.W << 8) | (data & 0xFF);
		};

		scheduler.read(addr, size_type::BYTE, on_read);
		scheduler.read(addr + 2, size_type::BYTE, on_read);

		if(size == size_type::LONG)
		{
			scheduler.read(addr + 4, size_type::BYTE, on_read);
			scheduler.read(addr + 6, size_type::BYTE, on_read);
		}
	}

	void movep_register_to_memory()
	{
		std::uint32_t data = regs.D(dest_reg).LW;

		if(size == size_type::WORD)
		{
			scheduler.write(addr, data >> 8, size_type::BYTE);
			scheduler.write(addr + 2, data, size_type::BYTE);
		}
		else
		{
			scheduler.write(addr, data >> 24, size_type::BYTE);
			scheduler.write(addr + 2, data >> 16, size_type::BYTE);
			scheduler.write(addr + 4, data >> 8, size_type::BYTE);
			scheduler.write(addr + 6, data, size_type::BYTE);
		}
	}

	exec_state move_from_sr_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
		{
			res = regs.SR; // TODO: do I need to set unimplemented bits to zero?
			auto op = dec.result();
			schedule_prefetch_and_write(op, res, size_type::WORD);
			scheduler.wait(timings::move_from_sr(op));
			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state move_to_sr_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			if(!in_supervisory())
				throw not_implemented();

			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
			regs.SR = operations::move_to_sr(dec.result());
			scheduler.wait(4);
			regs.PC -= 2;
			scheduler.prefetch_irc();
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state move_usp_handler()
	{
		if(!in_supervisory())
			throw not_implemented();
		
		std::uint8_t dr = (opcode >> 3) & 1;
		std::uint8_t reg = opcode & 0x7;

		// USP -> address register
		if(dr == 1)
		{
			regs.A(reg).LW = regs.USP.LW;
		}
		// address register -> USP
		else
		{
			regs.USP.LW = regs.A(reg).LW;
		}

		scheduler.prefetch_one();
		return exec_state::done;
	}

	exec_state move_to_ccr_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
			regs.SR = operations::move_to_ccr(dec.result(), regs.SR);
			scheduler.wait(4);
			regs.PC -= 2;
			scheduler.prefetch_irc();
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state alu_to_ccr_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			read_imm(size_type::BYTE);
			return exec_state::wait_scheduler;

		case 1:
			operations::alu_to_ccr(curr_inst, imm & 0xFF, regs.SR);
			scheduler.wait(8);
			regs.PC -= 2;
			scheduler.prefetch_irc();
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state alu_to_sr_handler()
	{
		if(!in_supervisory())
			throw not_implemented();
		
		switch (exec_stage++)
		{
		case 0:
			read_imm(size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
			operations::alu_to_sr(curr_inst, imm, regs.SR);
			scheduler.wait(8);
			regs.PC -= 2;
			scheduler.prefetch_irc();
			scheduler.prefetch_one();
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state shift_reg_handler()
	{
		bool ir = bit_is_set(opcode, 5);
		std::uint8_t count_or_reg = (opcode >> 9) & 0x7;

		std::uint32_t shift_count;
		if(ir)
		{
			shift_count = regs.D(count_or_reg).B;
		}
		else
		{
			if(count_or_reg == 0)
				count_or_reg = 8;
			shift_count = count_or_reg;
		}

		size = dec_size(opcode >> 6);
		auto& reg = regs.D(opcode & 0x7);
		bool is_left_shift = bit_is_set(opcode, 8);
		
		// std::cout << "Shifting " << (int)reg.B << " by " << (int)shift_count << std::endl;
		res = operations::shift(curr_inst, reg, shift_count, is_left_shift, size, regs.flags);
		store(reg, size, res);

		scheduler.prefetch_one();
		scheduler.wait(timings::reg_shift(shift_count, size));
		return exec_state::done;
	}

	exec_state shift_mem_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto op = dec.result();
			bool is_left_shift = bit_is_set(opcode, 8);
			res = operations::shift(curr_inst, op, 1, is_left_shift, size_type::WORD, regs.flags);
			
			schedule_prefetch_and_write(op, res, size_type::WORD);
		}
			return exec_state::done;

		default: throw internal_error();
		}
	}

	exec_state tst_handler()
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

			operations::tst(op, size, regs.flags);
			scheduler.prefetch_one();

			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state mulu_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto op = dec.result();
			auto& dest = regs.D((opcode >> 9) & 7);
			std::uint32_t src = operations::value(op, size_type::WORD);

			res = operations::alu(curr_inst, src, dest, size_type::WORD, regs.flags);
			store(dest, size_type::LONG, res);

			scheduler.prefetch_one();
			scheduler.wait(timings::mul(curr_inst, src));

			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state trap_handler()
	{
		std::uint8_t vector = 32 + (opcode & 0b1111);
		exman.rise_trap(vector);
		return exec_state::done;
	}

	exec_state trapv_handler()
	{
		scheduler.prefetch_one();
		if(regs.flags.V == 1)
		{
			exman.rise_trap(7);
		}

		return exec_state::done;
	}

	exec_state div_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			dec.schedule_decoding(opcode & 0xFF, size_type::WORD);
			return exec_state::wait_scheduler;

		case 1:
		{
			auto& dest_reg = regs.D((opcode >> 9) & 0x7);
			auto op = dec.result();

			std::uint32_t dest = dest_reg.LW;
			std::uint16_t src = operations::value(op, size_type::WORD);

			if(src == 0)
			{
				operations::divu_zero_division(regs.flags);
				exman.rise_division_by_zero();
			}
			else
			{
				dest_reg.LW = operations::alu(curr_inst, dest, src, size_type::LONG, regs.flags);
				scheduler.wait(timings::div(curr_inst, dest, src));
				scheduler.prefetch_one();
			}

			return exec_state::done;
		}

		default: throw internal_error();
		}
	}

	exec_state ext_handler()
	{
		auto& reg = regs.D(opcode & 0x7);
		std::uint8_t opmode = (opcode >> 6) & 1;
		size = bit_is_set(opcode, 6) ? size_type::WORD : size_type::BYTE;

		res = operations::ext(reg, size, regs.flags);
		size_type new_size = size == size_type::WORD ? size_type::LONG : size_type::WORD;

		store(reg, new_size, res);

		scheduler.prefetch_one();
		return exec_state::done;
	}

	// save the result (write to register or to memory), do prefetch
	void schedule_prefetch_and_write(operand& op, std::uint32_t res, size_type size)
	{
		scheduler.prefetch_one();
		if(!op.is_pointer())
		{
			store(op, size, res);
		}
		else
		{
			scheduler.write(op.pointer().address, res, size);
		}
	}

private:
	void store(data_register& d, size_type size, std::uint32_t res)
	{
		if(size == size_type::BYTE)
		{
			d.B = (std::uint8_t)res;
		}
		else if(size == size_type::WORD)
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

	void store(operand& op, size_type size, std::uint32_t res)
	{
		if(size == size_type::BYTE)
		{
			if(op.is_data_reg())
				op.data_reg().B = (std::uint8_t)res;
			else
				throw_invalid_opcode();
		}
		else if(size == size_type::WORD)
		{
			if(op.is_data_reg())
				op.data_reg().W = (std::uint16_t)res;
			else if(op.is_addr_reg())
				op.addr_reg().W = (std::uint16_t)res;
		}
		else if(size == size_type::LONG)
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
	size_type dec_size(std::uint8_t size)
	{
		size = size & 0b11;
		if(size == 0) return size_type::BYTE;
		if(size == 1) return size_type::WORD;
		if(size == 2) return size_type::LONG;

		throw_invalid_opcode();
	}

	size_type dec_move_size(std::uint8_t size)
	{
		size = size & 0b11;
		if(size == 0b01) return size_type::BYTE;
		if(size == 0b11) return size_type::WORD;
		if(size == 0b10) return size_type::LONG;

		throw_invalid_opcode();
	}

	inst_type decode_opcode(std::uint16_t opcode)
	{
		auto res = opcode_decoder::decode(opcode);
		if(res == inst_type::NONE)
			throw not_implemented();
		return res;
	}

	static bool bit_is_set(std::uint32_t data, std::uint8_t bit_number)
	{
		return ((data >> bit_number) & 1) == 1;
	}

	bool in_supervisory() const
	{
		return regs.flags.S == 1;
	}

private:
	m68k::ea_decoder dec;
	exception_manager& exman;

	std::uint16_t opcode = 0;
	inst_type curr_inst;
	std::uint8_t exec_stage;

	std::uint8_t state;

	// some helper variables
	std::uint32_t res = 0;
	std::uint32_t addr = 0;
	size_type size;
	std::uint8_t src_reg = 0;
	std::uint8_t dest_reg = 0;
	std::uint16_t move_reg_mask;
	status_register flags;
};

}

#endif // __M68K_INSTRUCTION_UNIT_HPP__
