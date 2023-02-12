#ifndef __GEN_HELPERS_HPP__
#define __GEN_HELPERS_HPP__

#include "../instructions.hpp"

#include <array>

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
	static std::array<const char*, 107> enum_str {
		"operation_type::unknown",
		"operation_type::add",
		"operation_type::adc",
		"operation_type::sub",
		"operation_type::sbc",
		"operation_type::and_8",
		"operation_type::or_8",
		"operation_type::xor_8",
		"operation_type::cp",
		"operation_type::inc_reg",
		"operation_type::inc_at",
		"operation_type::dec_reg",
		"operation_type::dec_at",
		"operation_type::add_16",
		"operation_type::adc_hl",
		"operation_type::sbc_hl",
		"operation_type::inc_reg_16",
		"operation_type::dec_reg_16",
		"operation_type::ld_reg",
		"operation_type::ld_at",
		"operation_type::ld_ir",
		"operation_type::ld_16_reg",
		"operation_type::ld_16_reg_from",
		"operation_type::ld_16_at",
		"operation_type::push",
		"operation_type::pop",
		"operation_type::call",
		"operation_type::call_cc",
		"operation_type::rst",
		"operation_type::ret",
		"operation_type::reti",
		"operation_type::retn",
		"operation_type::ret_cc",
		"operation_type::jp",
		"operation_type::jp_cc",
		"operation_type::jr_z",
		"operation_type::jr_nz",
		"operation_type::jr_c",
		"operation_type::jr_nc",
		"operation_type::jr",
		"operation_type::djnz",
		"operation_type::in",
		"operation_type::in_reg",
		"operation_type::in_c",
		"operation_type::ini",
		"operation_type::inir",
		"operation_type::ind",
		"operation_type::indr",
		"operation_type::out",
		"operation_type::out_reg",
		"operation_type::outi",
		"operation_type::otir",
		"operation_type::outd",
		"operation_type::otdr",
		"operation_type::ex_de_hl",
		"operation_type::ex_af_afs",
		"operation_type::exx",
		"operation_type::ex_16_at",
		"operation_type::ldi",
		"operation_type::ldir",
		"operation_type::cpd",
		"operation_type::cpdr",
		"operation_type::cpi",
		"operation_type::cpir",
		"operation_type::ldd",
		"operation_type::lddr",
		"operation_type::rlca",
		"operation_type::rrca",
		"operation_type::rla",
		"operation_type::rra",
		"operation_type::rld",
		"operation_type::rrd",
		"operation_type::rlc",
		"operation_type::rlc_at",
		"operation_type::rrc",
		"operation_type::rrc_at",
		"operation_type::rl",
		"operation_type::rl_at",
		"operation_type::rr",
		"operation_type::rr_at",
		"operation_type::sla",
		"operation_type::sla_at",
		"operation_type::sra",
		"operation_type::sra_at",
		"operation_type::srl",
		"operation_type::srl_at",
		"operation_type::sll",
		"operation_type::sll_at",
		"operation_type::tst_bit",
		"operation_type::tst_bit_at",
		"operation_type::set_bit",
		"operation_type::set_bit_at",
		"operation_type::res_bit",
		"operation_type::res_bit_at",
		"operation_type::bit_group",
		"operation_type::daa",
		"operation_type::cpl",
		"operation_type::neg",
		"operation_type::ccf",
		"operation_type::scf",
		"operation_type::di",
		"operation_type::ei",
		"operation_type::nop",
		"operation_type::halt",
		"operation_type::im0",
		"operation_type::im1",
		"operation_type::im2",
	};

	std::uint8_t val = op;
	return enum_str.at(val);
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
