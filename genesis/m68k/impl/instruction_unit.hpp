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
	};

public:
	instruction_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm,
		m68k::prefetch_queue& pq, m68k::bus_scheduler& scheduler)
		: base_unit(regs, busm, pq, scheduler), dec(regs, pq, scheduler)
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
	handler on_handler() override
	{
		switch (state)
		{
		case IDLE:
			opcode = pq.IRD;
			curr_inst = decode_opcode(opcode);
			// std::cout << "Executing: " << (int)curr_inst << std::endl;
			regs.PC += 2;
			state = EXECUTING;
			[[fallthrough]];

		case EXECUTING:
			return execute_handler();

		default:
			throw internal_error();
		}
	}

private:
	void execute()
	{
		handler req = execute_handler();

		if(req == handler::wait_scheduler)
			wait_scheduler();
		else if (req == handler::wait_scheduler_and_done)
			wait_scheduler_and_idle();
		else if(req == handler::in_progress)
			return; // just wait
		else
			throw internal_error();
	}

	handler execute_handler()
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

		default: throw internal_error();
		}
	}

	// TODO: in most cases we do not handle invalid opcodes properly
	// especially with some N/A EA decoding

	handler alu_mode_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			return handler::wait_scheduler;

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
			return handler::wait_scheduler_and_done;
		}

		default: throw internal_error();
		}
	}

	handler alu_address_mode_handler()
	{
		const std::uint8_t opmode = (opcode >> 6) & 0x7;
		if(opmode != 0b011 && opmode != 0b111)
			throw_invalid_opcode();

		switch (exec_stage++)
		{
		case 0:
			size = opmode == 0b011 ? 2 : 4;
			decode_ea(pq.IRD & 0xFF, size);
			return handler::wait_scheduler;

		case 1:
		{
			auto& reg = regs.A((opcode >> 9) & 0x7);
			auto op = dec.result();

			reg.LW = operations::alu(curr_inst, op, reg.LW, size, regs.flags);

			scheduler.prefetch_one();
			scheduler.wait(timings::alu_mode(curr_inst, opmode, op));
			return handler::wait_scheduler_and_done;
		}

		default: throw internal_error();
		}
	}

	handler alu_imm_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			read_imm(size);
			return handler::wait_scheduler;

		case 1:
			decode_ea(opcode & 0xFF, size);
			return handler::wait_scheduler;

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
			return handler::wait_scheduler_and_done;
		}

		default: throw internal_error();
		}
	}

	handler alu_quick_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			return handler::wait_scheduler;

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
			return handler::wait_scheduler_and_done;
		}

		default: throw internal_error();
		}
	}

	handler rm_postinc_handler()
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
				return handler::wait_scheduler;
			}
			else
			{
				// data register
				auto src = regs.D(src_reg);
				auto dest = regs.D(dest_reg);
				res = operations::alu(curr_inst, data, res, size, regs.flags);
				store(dest, size, res);
				scheduler.prefetch_one();
				return handler::wait_scheduler_and_done;
			}

		case 1:
			res = data;
			read(regs.A(dest_reg).LW, size);
			// FIXME: read may rise an exception, register is not inc in such a case
			regs.inc_addr(dest_reg, size);
			return handler::wait_scheduler;

		case 2:
			operations::alu(curr_inst, data, res, size, regs.flags);
			scheduler.prefetch_one();
			return handler::wait_scheduler_and_done;

		default: throw internal_error();
		}
	}

	handler rm_predec_handler()
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
				return handler::wait_scheduler;
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
				return handler::wait_scheduler_and_done;
			}

		/* Got here if it's an address register */
		case 1:
			dec_and_read(src_reg, size);
			return handler::wait_scheduler;

		case 2:
			res = data;
			dec_and_read(dest_reg, size);
			return handler::wait_scheduler;

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
			return handler::wait_scheduler_and_done;

		default: throw internal_error();
		}
	}

	handler unary_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			size = dec_size(opcode >> 6);
			decode_ea(pq.IRD & 0xFF, size);
			return handler::wait_scheduler;

		case 1:
		{
			auto op = dec.result();

			res = operations::alu(curr_inst, op, size, regs.flags);

			schedule_prefetch_and_write(op, res, size);
			scheduler.wait(timings::alu_size(curr_inst, size, op));

			return handler::wait_scheduler_and_done;
		}

		default: throw internal_error();
		}
	}

	handler nop_hanlder()
	{
		if(busm.is_idle())
		{
			scheduler.prefetch_one();
			return handler::wait_scheduler_and_done;
		}

		return handler::in_progress;
	}

	handler move_handler()
	{
		switch (exec_stage++)
		{
		case 0:
			// decode source
			size = dec_move_size(opcode >> 12);
			decode_ea(opcode & 0xFF, size);
			return handler::wait_scheduler;

		case 1:
		{
			src_op = dec.result();
			// decode destenation
			std::uint8_t ea = (opcode >> 6) & 7;
			ea = (ea << 3) | ((opcode >> 9) & 7);
			decode_ea(ea, size);
			return handler::wait_scheduler;
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
					scheduler.prefetch_one();
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
					scheduler.write(dest_op.pointer().address, res, (size_type)size);
				}
				else
				{
					store(dest_op, size, res);
					scheduler.prefetch_one();
					return handler::wait_scheduler_and_done;
				}
			}

			return handler::wait_scheduler;
		}

		case 3:
			if(((opcode >> 6) & 7) == 0b100)
			{
				scheduler.write(addr, res, (size_type)size);
			}
			else
			{
				scheduler.prefetch_one();
			}
			return handler::wait_scheduler_and_done;

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
	void decode_ea(std::uint8_t ea, std::uint8_t size)
	{
		dec.schedule_decoding(ea, size);
		if(dec.ready())
		{
			// immediate decoding
			execute();
		}
		else
		{
			// wait till decoding is over
			wait_scheduler();
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
