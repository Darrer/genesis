#ifndef __EXECUTIONER_HPP__
#define __EXECUTIONER_HPP__

#include "../cpu.h"
#include "operations.hpp"
#include "string_utils.hpp"

#include "instructions.hpp"

#include <cassert>
#include <iostream>

#include "decoder.hpp"


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

		for(const auto& inst : instructionss)
		{
			if(opcode == inst.opcodes[0] && (inst.opcodes[1] == 0x0 || inst.opcodes[1] == opcode2))
			{
				exec_inst(cpu, inst);
				return;
			}
		}

		throw std::runtime_error("decoder::execute_one error, unsupported opcode(" + su::hex_str(opcode) + 
			") at " + su::hex_str(regs.PC));
	}

private:
	static void exec_inst(z80::cpu& cpu, const z80::instruction& inst)
	{
		auto& regs = cpu.registers();
		auto& mem = cpu.memory();
		switch (inst.source)
		{
		case addressing_mode::register_a:
		case addressing_mode::register_b:
		case addressing_mode::register_c:
		case addressing_mode::register_d:
		case addressing_mode::register_e:
		case addressing_mode::register_h:
		case addressing_mode::register_l:
			exec_n(cpu, inst, decoder::decode_register(inst.source, regs));
			break;

		case addressing_mode::immediate:
			exec_n(cpu, inst, decoder::decode_immediate(inst, regs, mem));
			break;

		case addressing_mode::indirect_hl:
			exec_addr(cpu, inst, decoder::decode_indirect(inst.source, regs));
			break;

		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			exec_addr(cpu, inst, decoder::decode_indexed(inst.source, regs, mem));
			break;

		case addressing_mode::implied:
			exec_implied(cpu, inst);
			break;

		default:
			throw std::runtime_error("exec_inst error: unsupported source addresing mode: " + inst.source);
		}

		decoder::advance_pc(inst, regs);
	}

	static void exec_addr(z80::cpu& cpu, const z80::instruction& inst, z80::memory::address addr)
	{
		auto& regs = cpu.registers();
		auto& mem = cpu.memory();
		switch (inst.destination)
		{
		case addressing_mode::implied:
		{
			std::int8_t src = mem.read<std::int8_t>(addr);
			exec_unary(inst.op_type, regs, src);
			break;
		}

		case addressing_mode::indirect_hl:
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
		{
			// TODO: assume src=dest!!!
			exec_unary_addr(inst.op_type, regs, addr, mem);
			break;
		}
			
		default:
			throw std::runtime_error("exec_addr error: unsupported source addresing mode: " + inst.source);
		}
	}

	// TODO: src should be reference!
	static void exec_n(z80::cpu& cpu, const z80::instruction& inst, std::int8_t src)
	{
		auto& regs = cpu.registers();
		switch (inst.destination)
		{
		case addressing_mode::register_a:
		case addressing_mode::register_b:
		case addressing_mode::register_c:
		case addressing_mode::register_d:
		case addressing_mode::register_e:
		case addressing_mode::register_h:
		case addressing_mode::register_l:
		{
			auto& dest = decoder::decode_register(inst.destination, regs);
			exec_binary(inst.op_type, regs, src, dest);
			break;
		}

		case addressing_mode::implied:
			exec_unary(inst.op_type, regs, src);
			break;

		default:
			throw std::runtime_error("exec_n error: unsupported source addresing mode: " + inst.source);
		}
	}

	static void exec_implied(z80::cpu& cpu, const z80::instruction& inst)
	{
		auto& regs = cpu.registers();
		switch (inst.destination)
		{
		case addressing_mode::register_a:
		case addressing_mode::register_b:
		case addressing_mode::register_c:
		case addressing_mode::register_d:
		case addressing_mode::register_e:
		case addressing_mode::register_h:
		case addressing_mode::register_l:
			exec_unary(inst.op_type, regs, decoder::decode_register(inst.destination, regs));
			break;
		
		default:
			throw std::runtime_error("exec_implied error: unsupported source addresing mode: " + inst.source);
		}
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

	static void exec_binary(operation_type op_type, z80::cpu_registers&, std::int8_t& src, std::int8_t& dest)
	{
		switch (op_type)
		{
		case operation_type::ld_reg:
			operations::ld_reg(dest, src);
			break;
		
		default:
			throw std::runtime_error("exec_binary error: unknown/unsupported operation_type " + (int)op_type);
		}
	}
};

}

#endif // __EXECUTIONER_HPP__
