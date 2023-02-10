#ifndef __GEN_HELPERS_HPP__
#define __GEN_HELPERS_HPP__

#include "../instructions.hpp"

bool long_instruction(std::uint8_t op)
{
	switch (op)
	{
	case 0xDD:
	case 0xFD:
	case 0xED:
	case 0xCB:
		return true;
	default:
		return false;
	}
}

const char* operation_type_str(genesis::z80::operation_type op)
{
	using namespace genesis::z80;
	switch(op)
	{
	case operation_type::add:
		return "operation_type::add";
	case operation_type::adc:
		return "operation_type::adc";
	case operation_type::sub:
		return "operation_type::sub";
	case operation_type::sbc:
		return "operation_type::sbc";
	case operation_type::and_8:
		return "operation_type::and_8";
	case operation_type::or_8:
		return "operation_type::or_8";
	case operation_type::xor_8:
		return "operation_type::xor_8";
	case operation_type::cp:
		return "operation_type::cp";
	case operation_type::inc_reg:
		return "operation_type::inc_reg";
	case operation_type::inc_at:
		return "operation_type::inc_at";
	case operation_type::dec_reg:
		return "operation_type::dec_reg";
	case operation_type::dec_at:
		return "operation_type::dec_at";
	case operation_type::add_16:
		return "operation_type::add_16";
	case operation_type::adc_hl:
		return "operation_type::adc_hl";
	case operation_type::sbc_hl:
		return "operation_type::sbc_hl";
	case operation_type::inc_reg_16:
		return "operation_type::inc_reg_16";
	case operation_type::dec_reg_16:
		return "operation_type::dec_reg_16";
	case operation_type::ld_reg:
		return "operation_type::ld_reg";
	case operation_type::ld_at:
		return "operation_type::ld_at";
	case operation_type::ld_ir:
		return "operation_type::ld_ir";
	case operation_type::ld_16_reg:
		return "operation_type::ld_16_reg";
	case operation_type::ld_16_reg_from:
		return "operation_type::ld_16_reg_from";
	case operation_type::ld_16_at:
		return "operation_type::ld_16_at";
	case operation_type::push:
		return "operation_type::push";
	case operation_type::pop:
		return "operation_type::pop";
	case operation_type::call:
		return "operation_type::call";
	case operation_type::call_cc:
		return "operation_type::call_cc";
	case operation_type::rst:
		return "operation_type::rst";
	case operation_type::ret:
		return "operation_type::ret";
	case operation_type::reti:
		return "operation_type::reti";
	case operation_type::retn:
		return "operation_type::retn";
	case operation_type::ret_cc:
		return "operation_type::ret_cc";
	case operation_type::jp:
		return "operation_type::jp";
	case operation_type::jp_cc:
		return "operation_type::jp_cc";
	case operation_type::jr_z:
		return "operation_type::jr_z";
	case operation_type::jr_nz:
		return "operation_type::jr_nz";
	case operation_type::jr_c:
		return "operation_type::jr_c";
	case operation_type::jr_nc:
		return "operation_type::jr_nc";
	case operation_type::jr:
		return "operation_type::jr";
	case operation_type::djnz:
		return "operation_type::djnz";
	case operation_type::di:
		return "operation_type::di";
	case operation_type::ei:
		return "operation_type::ei";
	case operation_type::nop:
		return "operation_type::nop";
	case operation_type::halt:
		return "operation_type::halt";
	case operation_type::im0:
		return "operation_type::im0";
	case operation_type::im1:
		return "operation_type::im1";
	case operation_type::im2:
		return "operation_type::im2";
	case operation_type::out:
		return "operation_type::out";
	case operation_type::ex_de_hl:
		return "operation_type::ex_de_hl";
	case operation_type::ex_af_afs:
		return "operation_type::ex_af_afs";
	case operation_type::exx:
		return "operation_type::exx";
	case operation_type::ex_16_at:
		return "operation_type::ex_16_at";
	case operation_type::ldi:
		return "operation_type::ldi";
	case operation_type::ldir:
		return "operation_type::ldir";
	case operation_type::cpd:
		return "operation_type::cpd";
	case operation_type::cpdr:
		return "operation_type::cpdr";
	case operation_type::cpi:
		return "operation_type::cpi";
	case operation_type::cpir:
		return "operation_type::cpir";
	case operation_type::ldd:
		return "operation_type::ldd";
	case operation_type::lddr:
		return "operation_type::lddr";
	case operation_type::rlca:
		return "operation_type::rlca";
	case operation_type::rrca:
		return "operation_type::rrca";
	case operation_type::rla:
		return "operation_type::rla";
	case operation_type::rra:
		return "operation_type::rra";
	case operation_type::rld:
		return "operation_type::rld";
	case operation_type::rrd:
		return "operation_type::rrd";
	case operation_type::rlc:
		return "operation_type::rlc";
	case operation_type::rlc_at:
		return "operation_type::rlc_at";
	case operation_type::rrc:
		return "operation_type::rrc";
	case operation_type::rrc_at:
		return "operation_type::rrc_at";
	case operation_type::rl:
		return "operation_type::rl";
	case operation_type::rl_at:
		return "operation_type::rl_at";
	case operation_type::rr:
		return "operation_type::rr";
	case operation_type::rr_at:
		return "operation_type::rr_at";
	case operation_type::sla:
		return "operation_type::sla";
	case operation_type::sla_at:
		return "operation_type::sla_at";
	case operation_type::sra:
		return "operation_type::sra";
	case operation_type::sra_at:
		return "operation_type::sra_at";
	case operation_type::srl:
		return "operation_type::srl";
	case operation_type::srl_at:
		return "operation_type::srl_at";
	case operation_type::sll:
		return "operation_type::sll";
	case operation_type::sll_at:
		return "operation_type::sll_at";
	case operation_type::tst_bit:
		return "operation_type::tst_bit";
	case operation_type::tst_bit_at:
		return "operation_type::tst_bit_at";
	case operation_type::set_bit:
		return "operation_type::set_bit";
	case operation_type::set_bit_at:
		return "operation_type::set_bit_at";
	case operation_type::res_bit:
		return "operation_type::res_bit";
	case operation_type::res_bit_at:
		return "operation_type::res_bit_at";
	case operation_type::bit_group:
		return "operation_type::bit_group";
	case operation_type::daa:
		return "operation_type::daa";
	case operation_type::cpl:
		return "operation_type::cpl";
	case operation_type::neg:
		return "operation_type::neg";
	case operation_type::ccf:
		return "operation_type::ccf";
	case operation_type::scf:
		return "operation_type::scf";
	default:
		throw std::runtime_error("operation_type_str: unsupported operation_type!");
	}
}

const char* addressing_mode_str(genesis::z80::addressing_mode addr_mode)
{
	using namespace genesis::z80;
	switch (addr_mode)
	{
	case addressing_mode::none:
		return "addressing_mode::none";
	case addressing_mode::implied:
		return "addressing_mode::implied";
	case addressing_mode::register_a:
		return "addressing_mode::register_a";
	case addressing_mode::register_b:
		return "addressing_mode::register_b";
	case addressing_mode::register_c:
		return "addressing_mode::register_c";
	case addressing_mode::register_d:
		return "addressing_mode::register_d";
	case addressing_mode::register_e:
		return "addressing_mode::register_e";
	case addressing_mode::register_h:
		return "addressing_mode::register_h";
	case addressing_mode::register_l:
		return "addressing_mode::register_l";
	case addressing_mode::register_i:
		return "addressing_mode::register_i";
	case addressing_mode::register_r:
		return "addressing_mode::register_r";
	case addressing_mode::register_af:
		return "addressing_mode::register_af";
	case addressing_mode::register_bc:
		return "addressing_mode::register_bc";
	case addressing_mode::register_de:
		return "addressing_mode::register_de";
	case addressing_mode::register_hl:
		return "addressing_mode::register_hl";
	case addressing_mode::register_sp:
		return "addressing_mode::register_sp";
	case addressing_mode::register_ix:
		return "addressing_mode::register_ix";
	case addressing_mode::register_iy:
		return "addressing_mode::register_iy";
	case addressing_mode::register_ixh:
		return "addressing_mode::register_ixh";
	case addressing_mode::register_ixl:
		return "addressing_mode::register_ixl";
	case addressing_mode::register_iyh:
		return "addressing_mode::register_iyh";
	case addressing_mode::register_iyl:
		return "addressing_mode::register_iyl";
	case addressing_mode::immediate:
		return "addressing_mode::immediate";
	case addressing_mode::immediate_ext:
		return "addressing_mode::immediate_ext";
	case addressing_mode::indirect_hl:
		return "addressing_mode::indirect_hl";
	case addressing_mode::indirect_bc:
		return "addressing_mode::indirect_bc";
	case addressing_mode::indirect_de:
		return "addressing_mode::indirect_de";
	case addressing_mode::indirect_sp:
		return "addressing_mode::indirect_sp";
	case addressing_mode::indexed_ix:
		return "addressing_mode::indexed_ix";
	case addressing_mode::indexed_iy:
		return "addressing_mode::indexed_iy";
	case addressing_mode::bit:
		return "addressing_mode::bit";
	case addressing_mode::immediate_bit:
		return "addressing_mode::immediate_bit";
	default:
		throw std::runtime_error("addressing_mode_str: unsupported addressing_mode!");
	}
}

#endif // __GEN_HELPERS_HPP__
