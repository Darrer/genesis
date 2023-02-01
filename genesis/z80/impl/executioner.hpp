#ifndef __EXECUTIONER_HPP__
#define __EXECUTIONER_HPP__

#include "z80/cpu.h"
#include "string_utils.hpp"

#include "instructions.hpp"
#include "decoder.hpp"
#include "operations.hpp"
#include "inst_finder.hpp"


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

		// auto inst = finder.linear_search(opcode, opcode2);
		auto inst = finder.fast_search(opcode, opcode2);
		exec_inst(inst);
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
		case operation_type::add_16:
			ops.add_16(dec.decode_reg_16(inst.source), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::adc_hl:
			ops.adc_hl(dec.decode_reg_16(inst.source));
			break;
		case operation_type::sbc_hl:
			ops.sbc_hl(dec.decode_reg_16(inst.source));
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
		case operation_type::ld_16_at:
			ops.ld_at(dec.decode_2_bytes(inst.source, inst), dec.decode_address(inst.destination, inst));
			break;
		case operation_type::ld_ir:
			ops.ld_ir(dec.decode_reg_8(inst.source));
			break;
		case operation_type::ld_16_reg:
			ops.ld_reg(dec.decode_2_bytes(inst.source, inst), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::ld_16_reg_from:
			ops.ld_reg_from(dec.decode_reg_16(inst.destination), dec.decode_address(inst.source, inst));
			break;
		case operation_type::push:
			ops.push(dec.decode_reg_16(inst.source));
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
		case operation_type::rst:
			ops.rst(dec.decode_cc(inst));
			return;
		case operation_type::ret:
			ops.ret();
			return;
		case operation_type::ret_cc:
			ops.ret_cc(dec.decode_cc(inst));
			return;

		/* Jump Group */
		case operation_type::jp:
			ops.jp(dec.decode_address(inst.source, inst));
			return;
		case operation_type::jp_cc:
			ops.jp_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));
			return;
		case operation_type::jr:
			ops.jr(dec.decode_byte(inst.source, inst));
			return;
		case operation_type::jr_z:
			ops.jr_z(dec.decode_byte(inst.source, inst));
			return;
		case operation_type::jr_nz:
			ops.jr_nz(dec.decode_byte(inst.source, inst));
			return;
		case operation_type::jr_c:
			ops.jr_c(dec.decode_byte(inst.source, inst));
			return;
		case operation_type::jr_nc:
			ops.jr_nc(dec.decode_byte(inst.source, inst));
			return;

		/* CPU Control Groups */
		case operation_type::di:
			ops.di();
			break;
		case operation_type::ei:
			ops.ei();
			break;
		case operation_type::nop:
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
		case operation_type::ldir:
			ops.ldir();
			return;
		case operation_type::cpd:
			ops.cpd();
			break;
		case operation_type::cpdr:
			ops.cpdr();
			return;
		case operation_type::cpi:
			ops.cpi();
			break;
		case operation_type::cpir:
			ops.cpir();
			return;

		/* Rotate and Shift Group */
		case operation_type::rlca:
			ops.rlca();
			break;
		case operation_type::rrca:
			ops.rrca();
			break;
		case operation_type::rla:
			ops.rla();
			break;
		case operation_type::rra:
			ops.rra();
			break;

		/* Bit Set, Reset, and Test Group */
		case operation_type::tst_bit:
			ops.tst_bit(dec.decode_byte(inst.source, inst), dec.decode_bit(inst.destination, inst));
			break;

		case operation_type::neg:
			ops.neg();
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
	z80::inst_finder finder;
};

}

#endif // __EXECUTIONER_HPP__
