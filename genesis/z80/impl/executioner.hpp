#ifndef __EXECUTIONER_HPP__
#define __EXECUTIONER_HPP__

#include "../cpu.h"
#include "dispatching_tables.hpp"
#include "operations.hpp"
#include "string_utils.hpp"

#include <cassert>


namespace genesis::z80
{

class executioner
{
public:
	static void execute_one(z80::cpu& cpu)
	{
		auto mem = cpu.memory();
		auto& regs = cpu.registers();

		z80::opcode opcode = mem.read<z80::opcode>(regs.PC);
		z80::opcode opcode2 = mem.read<z80::opcode>(regs.PC + 1);

		// TODO: refine it!

		for(const auto& op : register_ops)
		{
			if(op.opcode == opcode)
			{
				execute_register_op(op, cpu);
				return;
			}
		}

		for(const auto& op : immediate_ops)
		{
			if(op.opcode == opcode)
			{
				exec_immediate_op(op, cpu);
				return;
			}
		}

		for(const auto& op : indirect_ops)
		{
			if(op.opcode == opcode)
			{
				if(op.access_type == access::read)
					exec_indirect_read_operation(op, cpu);
				else
					exec_indirect_write_operation(op, cpu);
				return;
			}
		}

		for(const auto& op : indexed_ops)
		{
			if(op.opcode == opcode && op.opcode2 == opcode2)
			{
				if(op.access_type == access::read)
					exec_indexed_read_operation(op, cpu);
				else
					exec_indexed_write_operation(op, cpu);
				return;
			}
		}
		
		throw std::runtime_error("decoder::execute_one error, unsupported opcode(" + su::hex_str(opcode) + 
			") at " + su::hex_str(regs.PC));
	}

	static void execute_register_op(register_operation reg_op, z80::cpu& cpu)
	{
		auto& regs = cpu.registers();
		switch(reg_op.reg)
		{
		case register_type::A:
			exec_unary(reg_op.op_type, regs, regs.main_set.A);
			break;
		case register_type::B:
			exec_unary(reg_op.op_type, regs, regs.main_set.B);
			break;
		case register_type::C:
			exec_unary(reg_op.op_type, regs, regs.main_set.C);
			break;
		case register_type::D:
			exec_unary(reg_op.op_type, regs, regs.main_set.D);
			break;
		case register_type::E:
			exec_unary(reg_op.op_type, regs, regs.main_set.E);
			break;
		case register_type::H:
			exec_unary(reg_op.op_type, regs, regs.main_set.H);
			break;
		case register_type::L:
			exec_unary(reg_op.op_type, regs, regs.main_set.L);
			break;
		default:
			throw std::runtime_error("execute_register_op unknown register type: " + reg_op.reg);
		}

		regs.PC += 1;
	}

	static void exec_immediate_op(immediate_operation im_op, z80::cpu& cpu)
	{
		auto& regs = cpu.registers();
		std::int8_t b = cpu.memory().read<std::int8_t>(regs.PC + 1);

		exec_unary(im_op.op_type, regs, b);
		regs.PC += 2;
	}

	static void exec_indirect_read_operation(indirect_operation ind_op, z80::cpu& cpu)
	{
		auto& regs = cpu.registers();
		std::int8_t b = cpu.memory().read<std::int8_t>(regs.main_set.HL);

		exec_unary(ind_op.op_type, regs, b);
		regs.PC += 1;
	}

	static void exec_indirect_write_operation(indirect_operation ind_op, z80::cpu& cpu)
	{
		auto& regs = cpu.registers();
		exec_unary_addr(ind_op.op_type, regs, regs.main_set.HL, cpu.memory());
		regs.PC += 1;
	}

	static void exec_indexed_read_operation(indexed_operation idx_op, z80::cpu& cpu)
	{
		assert(idx_op.opcode == 0xDD || idx_op.opcode == 0xFD);

		auto& regs = cpu.registers();
		auto d = cpu.memory().read<std::int8_t>(regs.PC + 2);
		auto base = idx_op.opcode == 0xDD ? regs.IX : regs.IY;

		std::int8_t b = cpu.memory().read<std::int8_t>(base + d);
		exec_unary(idx_op.op_type, regs, b);
		regs.PC += 3;
	}

	static void exec_indexed_write_operation(indexed_operation idx_op, z80::cpu& cpu)
	{
		assert(idx_op.opcode == 0xDD || idx_op.opcode == 0xFD);

		auto& regs = cpu.registers();
		auto d = cpu.memory().read<std::int8_t>(regs.PC + 2);
		auto base = idx_op.opcode == 0xDD ? regs.IX : regs.IY;

		exec_unary_addr(idx_op.op_type, regs, base + d, cpu.memory());
		regs.PC += 3;
	}

	static void exec_unary(operation_type op_type, z80::cpu_registers& regs, std::int8_t& b)
	{
		switch(op_type)
		{
		case operation_type::add:
			operations::add(regs, b);
			break;
		case operation_type::adc:
			operations::adc(regs, b);
			break;
		case operation_type::sub:
			operations::sub(regs, b);
			break;
		case operation_type::sbc:
			operations::sbc(regs, b);
			break;
		case operation_type::and_8:
			operations::and_8(regs, b);
			break;
		case operation_type::or_8:
			operations::or_8(regs, b);
			break;
		case operation_type::xor_8:
			operations::xor_8(regs, b);
			break;
		case operation_type::cp:
			operations::cp(regs, b);
			break;
		case operation_type::inc_reg:
			operations::inc_reg(regs, b);
			break;
		case operation_type::dec_reg:
			operations::dec_reg(regs, b);
			break;
		default:
			throw std::runtime_error("exec_unary_op error: unknown/unsupported operation_type " + (int)op_type);
		}
	}

	static void exec_unary_addr(operation_type op_type, z80::cpu_registers& regs, z80::memory::address addr, z80::memory& mem)
	{
		switch (op_type)
		{
		case operation_type::inc_at:
			operations::inc_at(regs, addr, mem);
			break;
		case operation_type::dec_at:
			operations::dec_at(regs, addr, mem);
			break;
		
		default:
			throw std::runtime_error("exec_unary_addr error: unknown/unsupported operation_type " + (int)op_type);
		}
	}
};

}

#endif // __EXECUTIONER_HPP__
