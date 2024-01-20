#include "helpers.h"

#include <iostream>
#include <functional>

using namespace genesis::z80;

class inline_decodern
{
	const char* decode_byte(addressing_mode);
	const char* decode_reg_8(addressing_mode);
};

// get_operation_call
const char* get_op_handler(genesis::z80::operation_type op/*source, destenation*/)
{
	using namespace genesis::z80;
	switch (op)
	{
	case operation_type::add:
		return "ops.add(dec.decode_byte(inst.source, inst));";
	case operation_type::adc:
		return "ops.adc(dec.decode_byte(inst.source, inst));";
	case operation_type::sub:
		return "ops.sub(dec.decode_byte(inst.source, inst));";
	case operation_type::sbc:
		return "ops.sbc(dec.decode_byte(inst.source, inst));";
	case operation_type::and_8:
		return "ops.and_8(dec.decode_byte(inst.source, inst));";
	case operation_type::or_8:
		return "ops.or_8(dec.decode_byte(inst.source, inst));";
	case operation_type::xor_8:
		return "ops.xor_8(dec.decode_byte(inst.source, inst));";
	case operation_type::cp:
		return "ops.cp(dec.decode_byte(inst.source, inst));";
	case operation_type::inc_reg:
		return "ops.inc_reg(dec.decode_reg_8(inst.source));";
	case operation_type::dec_reg:
		return "ops.dec_reg(dec.decode_reg_8(inst.source));";
	case operation_type::inc_at:
		return "ops.inc_at(dec.decode_address(inst.source, inst));";
	case operation_type::dec_at:
		return "ops.dec_at(dec.decode_address(inst.source, inst));";
	case operation_type::add_16:
		return "ops.add_16(dec.decode_reg_16(inst.source), dec.decode_reg_16(inst.destination));";
	case operation_type::adc_hl:
		return "ops.adc_hl(dec.decode_reg_16(inst.source));";
	case operation_type::sbc_hl:
		return "ops.sbc_hl(dec.decode_reg_16(inst.source));";
	case operation_type::inc_reg_16:
		return "ops.inc_reg_16(dec.decode_reg_16(inst.source));";
	case operation_type::dec_reg_16:
		return "ops.dec_reg_16(dec.decode_reg_16(inst.source));";
	case operation_type::ld_reg:
		return "ops.ld_reg(dec.decode_byte(inst.source, inst), dec.decode_reg_8(inst.destination));";
	case operation_type::ld_at:
		return "ops.ld_at(dec.decode_byte(inst.source, inst), dec.decode_address(inst.destination, inst));";
	case operation_type::ld_16_at:
		return "ops.ld_at(dec.decode_2_bytes(inst.source, inst), dec.decode_address(inst.destination, inst));";
	case operation_type::ld_ir:
		return "ops.ld_ir(dec.decode_reg_8(inst.source));";
	case operation_type::ld_16_reg:
		return "ops.ld_reg(dec.decode_2_bytes(inst.source, inst), dec.decode_reg_16(inst.destination));";
	case operation_type::ld_16_reg_from:
		return "ops.ld_reg_from(dec.decode_reg_16(inst.destination), dec.decode_address(inst.source, inst));";
	case operation_type::push:
		return "ops.push(dec.decode_reg_16(inst.source));";
	case operation_type::pop:
		return "ops.pop(dec.decode_reg_16(inst.destination));";
	case operation_type::call:
		return "ops.call(dec.decode_address(inst.source, inst));";
	case operation_type::call_cc:
		return "ops.call_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));";
	case operation_type::rst:
		return "ops.rst(dec.decode_cc(inst));";
	case operation_type::ret:
		return "ops.ret();";
	case operation_type::ret_cc:
		return "ops.ret_cc(dec.decode_cc(inst));";
	case operation_type::jp:
		return "ops.jp(dec.decode_address(inst.source, inst));";
	case operation_type::jp_cc:
		return "ops.jp_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));";
	case operation_type::jr:
		return "ops.jr(dec.decode_byte(inst.source, inst));";
	case operation_type::jr_z:
		return "ops.jr_z(dec.decode_byte(inst.source, inst));";
	case operation_type::jr_nz:
		return "ops.jr_nz(dec.decode_byte(inst.source, inst));";
	case operation_type::jr_c:
		return "ops.jr_c(dec.decode_byte(inst.source, inst));";
	case operation_type::jr_nc:
		return "ops.jr_nc(dec.decode_byte(inst.source, inst));";
	case operation_type::di:
		return "ops.di();";
	case operation_type::ei:
		return "ops.ei();";
	case operation_type::nop:
		return "";
	case operation_type::out:
		return "ops.out(dec.decode_byte(inst.source, inst));";
	case operation_type::ex_de_hl:
		return "ops.ex_de_hl();";
	case operation_type::ex_af_afs:
		return "ops.ex_af_afs();";
	case operation_type::exx:
		return "ops.exx();";
	case operation_type::ldi:
		return "ops.ldi();";
	case operation_type::ldir:
		return "ops.ldir();";
	case operation_type::cpd:
		return "ops.cpd();";
	case operation_type::cpdr:
		return "ops.cpdr();";
	case operation_type::cpi:
		return "ops.cpi();";
	case operation_type::cpir:
		return "ops.cpir();";
	case operation_type::ldd:
		return "ops.ldd();";
	case operation_type::lddr:
		return "ops.lddr();";
	case operation_type::rlca:
		return "ops.rlca();";
	case operation_type::rrca:
		return "ops.rrca();";
	case operation_type::rla:
		return "ops.rla();";
	case operation_type::rra:
		return "ops.rra();";
	case operation_type::rld:
		return "ops.rld();";
	case operation_type::rrd:
		return "ops.rrd();";
	case operation_type::rlc:
		return "ops.rlc(dec.decode_reg_8(inst.destination));";
	case operation_type::rlc_at:
		return "ops.rlc_at(dec.decode_address(inst.destination, inst));";
	case operation_type::rrc:
		return "ops.rrc(dec.decode_reg_8(inst.destination));";
	case operation_type::rrc_at:
		return "ops.rrc_at(dec.decode_address(inst.destination, inst));";
	case operation_type::rl:
		return "ops.rl(dec.decode_reg_8(inst.destination));";
	case operation_type::rl_at:
		return "ops.rl_at(dec.decode_address(inst.destination, inst));";
	case operation_type::rr:
		return "ops.rr(dec.decode_reg_8(inst.destination));";
	case operation_type::rr_at:
		return "ops.rr_at(dec.decode_address(inst.destination, inst));";
	case operation_type::sla:
		return "ops.sla(dec.decode_reg_8(inst.destination));";
	case operation_type::sla_at:
		return "ops.sla_at(dec.decode_address(inst.destination, inst));";
	case operation_type::sra:
		return "ops.sra(dec.decode_reg_8(inst.destination));";
	case operation_type::sra_at:
		return "ops.sra_at(dec.decode_address(inst.destination, inst));";
	case operation_type::srl:
		return "ops.srl(dec.decode_reg_8(inst.destination));";
	case operation_type::srl_at:
		return "ops.srl_at(dec.decode_address(inst.destination, inst));";
	case operation_type::sll:
		return "ops.sll(dec.decode_reg_8(inst.destination));";
	case operation_type::sll_at:
		return "ops.sll_at(dec.decode_address(inst.destination, inst));";
	case operation_type::tst_bit:
		return "ops.tst_bit(dec.decode_byte(inst.destination, inst), dec.decode_bit(inst.source, inst));";
	case operation_type::tst_bit_at:
		return "ops.tst_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));";
	case operation_type::set_bit:
		return "ops.set_bit(dec.decode_reg_8(inst.destination), dec.decode_bit(inst.source, inst));";
	case operation_type::set_bit_at:
		return "ops.set_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));";
	case operation_type::res_bit:
		return "ops.res_bit(dec.decode_reg_8(inst.destination), dec.decode_bit(inst.source, inst));";
	case operation_type::res_bit_at:
		return "ops.res_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));";
	case operation_type::bit_group:
		return "exec_bit_group(inst);";
	case operation_type::daa:
		return "ops.daa();";
	case operation_type::cpl:
		return "ops.cpl();";
	case operation_type::neg:
		return "ops.neg();";
	case operation_type::ccf:
		return "ops.ccf();";
	case operation_type::scf:
		return "ops.scf();";

	default:
		throw std::runtime_error("get_exec_op unsupporeted operatoin_type!");
	}
}

bool need_advance_pc(operation_type op)
{
	switch (op)
	{
	// these control PC itslef
	case operation_type::call:
	case operation_type::call_cc:
	case operation_type::rst:
	case operation_type::ret:
	case operation_type::ret_cc:
	case operation_type::jp:
	case operation_type::jp_cc:
	case operation_type::jr:
	case operation_type::jr_z:
	case operation_type::jr_nz:
	case operation_type::jr_c:
	case operation_type::jr_nc:
	case operation_type::ldir:
	case operation_type::cpdr:
	case operation_type::cpir:
	case operation_type::lddr:
	case operation_type::djnz:
		return false;
	default:
		return true;
	}
}

void print_break_or_return(operation_type op)
{
	if(need_advance_pc(op))
	{
		std::cout << "\t" << "break;" << std::endl;
	}
	else
	{
		std::cout << "\t" << "return;" << std::endl;
	}
}

void print_op_handler(operation_type op)
{
	std::cout << "\t" << get_op_handler(op) << std::endl;
	print_break_or_return(op);
}

void print_case_opcode(std::uint8_t op)
{
	std::cout << "case " << su::hex_str(op) << ":" << std::endl;
}

void print_group(const std::vector<instruction>& insts, std::uint8_t op_group = 0x0)
{
	if(op_group != 0x0)
	{
		print_case_opcode(op_group);
		std::cout << "{" << std::endl;
		std::cout << "switch (op2) {" << std::endl;
	}

	for(auto inst: insts)
	{
		std::uint8_t opcode = inst.opcodes[0];

		if(op_group != 0x0)
		{
			if(inst.opcodes[0] != op_group)
				continue;

			opcode = inst.opcodes[1];
		}
		else
		{
			if(long_instruction(opcode))
				continue;
		}

		print_case_opcode(opcode);
		print_op_handler(inst.op_type);
	}

	if(op_group != 0x0)
	{
		std::cout << "}" << std::endl;
		std::cout << "break;" << std::endl;
		std::cout << "}" << std::endl;
	}
}

// g++ -std=c++20 -I../../../ exec_generator.cpp -o exec_generator
int main()
{
	std::vector<instruction> insts (std::cbegin(instructions), std::cend(instructions));
	std::sort(insts.begin(), insts.end(), [](instruction a, instruction b)
	{
		std::uint8_t o1 = a.opcodes[0];
		std::uint8_t o2 = b.opcodes[0];
		if(o1 == o2)
		{
			o1 = a.opcodes[1];
			o2 = b.opcodes[1];
		}
		
		return o1 < o2;
	});

	
	print_group(insts);
	print_group(insts, 0xDD);
	print_group(insts, 0xFD);
	print_group(insts, 0xCB);
	print_group(insts, 0xED);

	return EXIT_SUCCESS;
}
