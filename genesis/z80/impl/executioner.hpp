#ifndef __EXECUTIONER_HPP__
#define __EXECUTIONER_HPP__

#include "z80/cpu.h"
#include "string_utils.hpp"

#include "instructions.hpp"
#include "decoder.hpp"
#include "operations.hpp"


namespace genesis::z80
{

class executioner
{
public:
	executioner(z80::cpu& cpu) : cpu(cpu), dec(z80::decoder(cpu)), ops(z80::operations(cpu))
	{
	}

	void execute_one()
	{
		auto& mem = cpu.memory();
		auto& regs = cpu.registers();

		z80::opcode opcode = mem.read<z80::opcode>(regs.PC);
		z80::opcode opcode2 = mem.read<z80::opcode>(regs.PC + 1);

		for(const auto& inst : instructions)
		{
			if(opcode == inst.opcodes[0] && (inst.opcodes[1] == 0x0 || inst.opcodes[1] == opcode2))
			{
				exec_inst(inst);
				return;
			}
		}

		throw std::runtime_error("execute_one error, unsupported opcode(" + su::hex_str(opcode) + 
			") at " + su::hex_str(regs.PC));
	}

private:
	void exec_inst(const z80::instruction& inst)
	{
		switch(inst.op_type)
		{
		/* 8-Bit Arithmetic Group */
		case operation_type::add:
			ops.add(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::adc:
			ops.adc(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sub:
			ops.sub(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sbc:
			ops.sbc(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::and_8:
			ops.and_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::or_8:
			ops.or_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::xor_8:
			ops.xor_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::cp:
			ops.cp(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::inc_reg:
			ops.inc_reg(dec.decode_reg_8(inst.source));
			break;
		case operation_type::dec_reg:
			ops.dec_reg(dec.decode_reg_8(inst.source));
			break;
		case operation_type::inc_at:
			ops.inc_at(dec.decode_address(inst.source, inst));
			break;
		case operation_type::dec_at:
			ops.dec_at(dec.decode_address(inst.source, inst));
			break;

		/* 16-Bit Arithmetic Group */
		case operation_type::add_hl:
			ops.add_hl(dec.decode_reg_16(inst.source));
			break;
		case operation_type::inc_reg_16:
			ops.inc_reg_16(dec.decode_reg_16(inst.source));
			break;
		case operation_type::dec_reg_16:
			ops.dec_reg_16(dec.decode_reg_16(inst.source));
			break;

		/* 8/16-Bit Load Group */
		case operation_type::ld_reg:
			ops.ld_reg(dec.decode_byte(inst.source, inst), dec.decode_reg_8(inst.destination));
			break;
		case operation_type::ld_at:
			ops.ld_at(dec.decode_byte(inst.source, inst), dec.decode_address(inst.destination, inst));
			break;
		case operation_type::ld_ir:
			ops.ld_ir(dec.decode_reg_8(inst.source));
			break;
		case operation_type::ld_16_reg:
			ops.ld_reg(dec.decode_2_bytes(inst.source, inst), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::push:
			ops.push(dec.decode_2_bytes(inst.source, inst));
			break;
		case operation_type::pop:
			ops.pop(dec.decode_reg_16(inst.destination));
			break;

		/* Call and Return Group */
		case operation_type::call:
			ops.call(dec.decode_address(inst.source, inst));
			return;
		case operation_type::call_cc:
			ops.call_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));
			return;
		case operation_type::ret:
			ops.ret();
			return;

		/* Jump Group */
		case operation_type::jp:
			ops.jp(dec.decode_address(inst.source, inst));
			return;
		case operation_type::jp_cc:
			ops.jp_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));
			return;
		case operation_type::jr_z:
			ops.jr_z(dec.decode_byte(inst.source, inst));
			return;
		case operation_type::jr:
			ops.jr(dec.decode_byte(inst.source, inst));
			return;

		/* CPU Control Groups */
		case operation_type::di:
			ops.di();
			break;
		case operation_type::ei:
			ops.ei();
			break;

		/* Input and Output Group */
		case operation_type::out:
			ops.out(dec.decode_byte(inst.source, inst));
			break;

		/* Exchange, Block Transfer, and Search Group */
		case operation_type::ex_de_hl:
			ops.ex_de_hl();
			break;
		case operation_type::ex_af_afs:
			ops.ex_af_afs();
			break;
		case operation_type::exx:
			ops.exx();
			break;

		default:
			throw std::runtime_error("exec_inst error: unsupported operation " + std::to_string(inst.op_type));
		}

		dec.advance_pc(inst);
	}

private:
	z80::cpu& cpu;
	z80::decoder dec;
	z80::operations ops;
};

}

#endif // __EXECUTIONER_HPP__
