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
		auto& mem = cpu.memory();
		auto& regs = cpu.registers();

		z80::opcode opcode = mem.read<z80::opcode>(regs.PC);
		z80::opcode opcode2 = mem.read<z80::opcode>(regs.PC + 1);

		for(const auto& inst : instructions)
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

	static void exec_inst(z80::cpu& cpu, const z80::instruction& inst)
	{
		auto& mem = cpu.memory();
		auto& regs = cpu.registers();

		switch(inst.op_type)
		{
		/* 8-Bit Arithmetic Group */
		case operation_type::add:
			operations::add(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::adc:
			operations::adc(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::sub:
			operations::sub(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::sbc:
			operations::sbc(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::and_8:
			operations::and_8(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::or_8:
			operations::or_8(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::xor_8:
			operations::xor_8(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::cp:
			operations::cp(regs, decoder::decode_to_byte(inst.source, inst, regs, mem));
			break;
		case operation_type::inc_reg:
			operations::inc_reg(regs, decoder::decode_register(inst.source, regs));
			break;
		case operation_type::dec_reg:
			operations::dec_reg(regs, decoder::decode_register(inst.source, regs));
			break;
		case operation_type::inc_at:
			operations::inc_at(regs, decoder::decode_address(inst.source, inst, regs, mem), mem);
			break;
		case operation_type::dec_at:
			operations::dec_at(regs, decoder::decode_address(inst.source, inst, regs, mem), mem);
			break;

		/* 8/16-Bit Load Group */
		case operation_type::ld_reg:
			operations::ld_reg(decoder::decode_to_byte(inst.source, inst, regs, mem), decoder::decode_register(inst.destination, regs));
			break;
		case operation_type::ld_at:
			operations::ld_at(decoder::decode_to_byte(inst.source, inst, regs, mem), decoder::decode_address(inst.destination, inst, regs, mem), mem);
			break;
		case operation_type::ld_ir:
			operations::ld_ir(decoder::decode_register(inst.source, regs), regs);
			break;
		case operation_type::ld_16_reg:
			operations::ld_reg(decoder::decode_two_bytes(inst.source, inst, regs, mem), decoder::decode_register_pair(inst.destination, regs));
			break;
		case operation_type::push:
			operations::push(decoder::decode_two_bytes(inst.source, inst, regs, mem), regs, mem);
			break;

		/* Call and Return Group */
		case operation_type::call:
			operations::call(decoder::decode_address(inst.source, inst, regs, mem), regs, mem);
			return;
		case operation_type::ret:
			operations::ret(regs, mem);
			return;
		
		/* Jump Group */
		case operation_type::jp:
			operations::jp(decoder::decode_address(inst.source, inst, regs, mem), regs);
			return;

		/* CPU Control Groups */
		case operation_type::di:
			operations::di();
			break;

		default:
			throw std::runtime_error("exec_inst error: unsupported operation " + inst.op_type);
		}

		decoder::advance_pc(inst, regs);
	}
};

}

#endif // __EXECUTIONER_HPP__
