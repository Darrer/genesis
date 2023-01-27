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

		throw std::runtime_error("executioner::execute_one error, unsupported opcode(" + su::hex_str(opcode) + 
			") at " + su::hex_str(regs.PC));
	}

	static void exec_inst(z80::cpu& cpu, const z80::instruction& inst)
	{
		z80::decoder dec = z80::decoder(cpu);

		auto& mem = cpu.memory();
		auto& regs = cpu.registers();

		switch(inst.op_type)
		{
		/* 8-Bit Arithmetic Group */
		case operation_type::add:
			operations::add(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::adc:
			operations::adc(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sub:
			operations::sub(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sbc:
			operations::sbc(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::and_8:
			operations::and_8(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::or_8:
			operations::or_8(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::xor_8:
			operations::xor_8(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::cp:
			operations::cp(regs, dec.decode_byte(inst.source, inst));
			break;
		case operation_type::inc_reg:
			operations::inc_reg(regs, dec.decode_reg_8(inst.source));
			break;
		case operation_type::dec_reg:
			operations::dec_reg(regs, dec.decode_reg_8(inst.source));
			break;
		case operation_type::inc_at:
			operations::inc_at(regs, dec.decode_address(inst.source, inst), mem);
			break;
		case operation_type::dec_at:
			operations::dec_at(regs, dec.decode_address(inst.source, inst), mem);
			break;

		/* 8/16-Bit Load Group */
		case operation_type::ld_reg:
			operations::ld_reg(dec.decode_byte(inst.source, inst), dec.decode_reg_8(inst.destination));
			break;
		case operation_type::ld_at:
			operations::ld_at(dec.decode_byte(inst.source, inst), dec.decode_address(inst.destination, inst), mem);
			break;
		case operation_type::ld_ir:
			operations::ld_ir(dec.decode_reg_8(inst.source), regs);
			break;
		case operation_type::ld_16_reg:
			operations::ld_reg(dec.decode_2_bytes(inst.source, inst), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::push:
			operations::push(dec.decode_2_bytes(inst.source, inst), regs, mem);
			break;

		/* Call and Return Group */
		case operation_type::call:
			operations::call(dec.decode_address(inst.source, inst), regs, mem);
			return;
		case operation_type::call_cc:
			operations::call_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst), regs, mem);
			return;
		case operation_type::ret:
			operations::ret(regs, mem);
			return;

		/* Jump Group */
		case operation_type::jp:
			operations::jp(dec.decode_address(inst.source, inst), regs);
			return;
		case operation_type::jr_z:
			operations::jr_z(dec.decode_byte(inst.source, inst), regs);
			return;
		case operation_type::jr:
			operations::jr(dec.decode_byte(inst.source, inst), regs);
			return;

		/* CPU Control Groups */
		case operation_type::di:
			operations::di();
			break;
		case operation_type::ei:
			operations::ei();
			break;

		default:
			throw std::runtime_error("exec_inst error: unsupported operation " + inst.op_type);
		}

		dec.advance_pc(inst);
	}
};

}

#endif // __EXECUTIONER_HPP__
