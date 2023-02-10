/*
 * Using instructions array generates, well..., another instructions array (but cleaner!).
*/

#include "../instructions.hpp"
#include "helpers.hpp"

#include <iostream>

using namespace genesis::z80;

enum register_type : std::uint8_t
{
	A = 0b111,
	B = 0b000,
	C = 0b001,
	D = 0b010,
	E = 0b011,
	H = 0b100,
	L = 0b101
};

// name pattern <source>_<destination>

#define destination_register_wide(op, opcode, opcode2, reg_offset, src) \
	{op, { opcode, opcode2 | (register_type::A << reg_offset) }, src, addressing_mode::register_a}, \
	{op, { opcode, opcode2 | (register_type::B << reg_offset) }, src, addressing_mode::register_b}, \
	{op, { opcode, opcode2 | (register_type::C << reg_offset) }, src, addressing_mode::register_c}, \
	{op, { opcode, opcode2 | (register_type::D << reg_offset) }, src, addressing_mode::register_d}, \
	{op, { opcode, opcode2 | (register_type::E << reg_offset) }, src, addressing_mode::register_e}, \
	{op, { opcode, opcode2 | (register_type::H << reg_offset) }, src, addressing_mode::register_h}, \
	{op, { opcode, opcode2 | (register_type::L << reg_offset) }, src, addressing_mode::register_l}

#define destination_register(op, opcode, reg_offset, src) \
	{op, { opcode | (register_type::A << reg_offset) }, src, addressing_mode::register_a}, \
	{op, { opcode | (register_type::B << reg_offset) }, src, addressing_mode::register_b}, \
	{op, { opcode | (register_type::C << reg_offset) }, src, addressing_mode::register_c}, \
	{op, { opcode | (register_type::D << reg_offset) }, src, addressing_mode::register_d}, \
	{op, { opcode | (register_type::E << reg_offset) }, src, addressing_mode::register_e}, \
	{op, { opcode | (register_type::H << reg_offset) }, src, addressing_mode::register_h}, \
	{op, { opcode | (register_type::L << reg_offset) }, src, addressing_mode::register_l}

#define source_register_wide(op, opcode, opcode2, reg_offset, dest) \
	{op, { opcode, opcode2 | (register_type::A << reg_offset) }, addressing_mode::register_a, dest}, \
	{op, { opcode, opcode2 | (register_type::B << reg_offset) }, addressing_mode::register_b, dest}, \
	{op, { opcode, opcode2 | (register_type::C << reg_offset) }, addressing_mode::register_c, dest}, \
	{op, { opcode, opcode2 | (register_type::D << reg_offset) }, addressing_mode::register_d, dest}, \
	{op, { opcode, opcode2 | (register_type::E << reg_offset) }, addressing_mode::register_e, dest}, \
	{op, { opcode, opcode2 | (register_type::H << reg_offset) }, addressing_mode::register_h, dest}, \
	{op, { opcode, opcode2 | (register_type::L << reg_offset) }, addressing_mode::register_l, dest}

#define source_register(op, opcode, reg_offset, dest) \
	{op, { opcode | (register_type::A << reg_offset) }, addressing_mode::register_a, dest}, \
	{op, { opcode | (register_type::B << reg_offset) }, addressing_mode::register_b, dest}, \
	{op, { opcode | (register_type::C << reg_offset) }, addressing_mode::register_c, dest}, \
	{op, { opcode | (register_type::D << reg_offset) }, addressing_mode::register_d, dest}, \
	{op, { opcode | (register_type::E << reg_offset) }, addressing_mode::register_e, dest}, \
	{op, { opcode | (register_type::H << reg_offset) }, addressing_mode::register_h, dest}, \
	{op, { opcode | (register_type::L << reg_offset) }, addressing_mode::register_l, dest}

#define source_bit(op, opcode1, opcode2, dest) \
	{op, {opcode1, opcode2 | (0b000 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b001 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b010 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b011 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b100 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b101 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b110 << 3)}, addressing_mode::bit, dest}, \
	{op, {opcode1, opcode2 | (0b111 << 3)}, addressing_mode::bit, dest}

#define ss_ss(op, opcode, reg_offset) \
	{op, {opcode | (0b00 << reg_offset)}, addressing_mode::register_bc, addressing_mode::register_bc}, \
	{op, {opcode | (0b01 << reg_offset)}, addressing_mode::register_de, addressing_mode::register_de}, \
	{op, {opcode | (0b10 << reg_offset)}, addressing_mode::register_hl, addressing_mode::register_hl}, \
	{op, {opcode | (0b11 << reg_offset)}, addressing_mode::register_sp, addressing_mode::register_sp}

#define cc_inst(op, opcode, src, dest) \
	{op, {opcode | (0b000 << 3)}, src, dest}, \
	{op, {opcode | (0b001 << 3)}, src, dest}, \
	{op, {opcode | (0b010 << 3)}, src, dest}, \
	{op, {opcode | (0b011 << 3)}, src, dest}, \
	{op, {opcode | (0b100 << 3)}, src, dest}, \
	{op, {opcode | (0b101 << 3)}, src, dest}, \
	{op, {opcode | (0b110 << 3)}, src, dest}, \
	{op, {opcode | (0b111 << 3)}, src, dest}

/* register source */

#define register_implied(op, opcode, reg_offset) source_register(op, opcode, reg_offset, addressing_mode::implied)
#define register_indirect_hl(op, opcode, reg_offset) source_register(op, opcode, reg_offset, addressing_mode::indirect_hl)
#define register_indexed(op, opcode2, reg_offset) \
	source_register_wide(op, 0xDD, opcode2, reg_offset, addressing_mode::indexed_ix), \
	source_register_wide(op, 0xFD, opcode2, reg_offset, addressing_mode::indexed_iy)

#define register_register(op, opcode) \
	source_register(op, opcode | (register_type::A << 3), 0, addressing_mode::register_a), \
	source_register(op, opcode | (register_type::B << 3), 0, addressing_mode::register_b), \
	source_register(op, opcode | (register_type::C << 3), 0, addressing_mode::register_c), \
	source_register(op, opcode | (register_type::D << 3), 0, addressing_mode::register_d), \
	source_register(op, opcode | (register_type::E << 3), 0, addressing_mode::register_e), \
	source_register(op, opcode | (register_type::H << 3), 0, addressing_mode::register_h), \
	source_register(op, opcode | (register_type::L << 3), 0, addressing_mode::register_l)

#define bit_register(op, opcode1, opcode2) \
	destination_register_wide(op, opcode1, opcode2 | (0b000 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b001 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b010 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b011 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b100 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b101 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b110 << 3), 0, addressing_mode::bit), \
	destination_register_wide(op, opcode1, opcode2 | (0b111 << 3), 0, addressing_mode::bit)


/* immediate source */

#define immediate_register(op, opcode, reg_offset) destination_register(op, opcode, reg_offset, addressing_mode::immediate)

#define immediate_indexed(op, opcode2) \
	{op, {0xDD, opcode2}, addressing_mode::immediate, addressing_mode::indexed_ix}, \
	{op, {0xFD, opcode2}, addressing_mode::immediate, addressing_mode::indexed_iy}

/* indirect source */

#define indirect_hl_register(op, opcode) destination_register(op, opcode, 3, addressing_mode::indirect_hl)

#define indirect_dd_des(op, opcode, dd_offset, src) \
	{op, {opcode | (0b00 << dd_offset)}, src, addressing_mode::register_bc}, \
	{op, {opcode | (0b01 << dd_offset)}, src, addressing_mode::register_de}, \
	{op, {opcode | (0b10 << dd_offset)}, src, addressing_mode::register_hl}, \
	{op, {opcode | (0b11 << dd_offset)}, src, addressing_mode::register_sp}

#define indexed_implied(op, opcode2) \
	{op, {0xDD, opcode2}, addressing_mode::indexed_ix, addressing_mode::implied}, \
	{op, {0xFD, opcode2}, addressing_mode::indexed_iy, addressing_mode::implied}

#define indexed_indexed(op, opcode2) \
	{op, {0xDD, opcode2}, addressing_mode::indexed_ix, addressing_mode::indexed_ix}, \
	{op, {0xFD, opcode2}, addressing_mode::indexed_iy, addressing_mode::indexed_iy}

#define indexed_register(op, opcode2) \
	destination_register_wide(op, 0xDD, opcode2, 3, addressing_mode::indexed_ix), \
	destination_register_wide(op, 0xFD, opcode2, 3, addressing_mode::indexed_iy)

#define ixhl_iyhl(op, opcode2, dest) \
	{op, {0xDD, opcode2},     addressing_mode::register_ixh, dest }, \
	{op, {0xDD, opcode2 + 1}, addressing_mode::register_ixl, dest }, \
	{op, {0xFD, opcode2},     addressing_mode::register_iyh, dest }, \
	{op, {0xFD, opcode2 + 1}, addressing_mode::register_iyl, dest }

#define ld_dd_reg_reg(op, opcode2, dest) \
	{op, {0xDD, (opcode2 | 0b000)}, addressing_mode::register_b, dest}, \
	{op, {0xDD, (opcode2 | 0b001)}, addressing_mode::register_c, dest}, \
	{op, {0xDD, (opcode2 | 0b010)}, addressing_mode::register_d, dest}, \
	{op, {0xDD, (opcode2 | 0b011)}, addressing_mode::register_e, dest}, \
	{op, {0xDD, (opcode2 | 0b100)}, addressing_mode::register_ixh, dest}, \
	{op, {0xDD, (opcode2 | 0b101)}, addressing_mode::register_ixl, dest}, \
	{op, {0xDD, (opcode2 | 0b111)}, addressing_mode::register_a, dest}

#define ld_fd_reg_reg(op, opcode2, dest) \
	{op, {0xFD, (opcode2 | 0b000)}, addressing_mode::register_b, dest}, \
	{op, {0xFD, (opcode2 | 0b001)}, addressing_mode::register_c, dest}, \
	{op, {0xFD, (opcode2 | 0b010)}, addressing_mode::register_d, dest}, \
	{op, {0xFD, (opcode2 | 0b011)}, addressing_mode::register_e, dest}, \
	{op, {0xFD, (opcode2 | 0b100)}, addressing_mode::register_iyh, dest}, \
	{op, {0xFD, (opcode2 | 0b101)}, addressing_mode::register_iyl, dest}, \
	{op, {0xFD, (opcode2 | 0b111)}, addressing_mode::register_a, dest}

#define ld_reg_reg(op, opcode2, dest) \
	ld_dd_reg_reg(op, opcode2, dest), \
	ld_fd_reg_reg(op, opcode2, dest)

instruction all_instructions[] = {
	/* 8-Bit Arithmetic Group */
	register_implied(operation_type::add, 0b10000000, 0),
	register_implied(operation_type::adc, 0b10001000, 0),
	register_implied(operation_type::sub, 0b10010000, 0),
	register_implied(operation_type::sbc, 0b10011000, 0),
	register_implied(operation_type::and_8, 0b10100000, 0),
	register_implied(operation_type::or_8, 0b10110000, 0),
	register_implied(operation_type::xor_8, 0b10101000, 0),
	register_implied(operation_type::cp, 0b10111000, 0),
	register_implied(operation_type::inc_reg, 0b00000100, 3),
	register_implied(operation_type::dec_reg, 0b00000101, 3),

	{ operation_type::add, {0xC6}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::adc, {0xCE}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::sub, {0xD6}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::sbc, {0xDE}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::and_8, {0xE6}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::or_8, {0xF6}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::xor_8, {0xEE}, addressing_mode::immediate, addressing_mode::implied },
	{ operation_type::cp, {0xFE}, addressing_mode::immediate, addressing_mode::implied },

	{ operation_type::add, {0x86}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::adc, {0x8E}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::sub, {0x96}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::sbc, {0x9E}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::and_8, {0xA6}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::or_8, {0xB6}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::xor_8, {0xAE}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::cp, {0xBE}, addressing_mode::indirect_hl, addressing_mode::implied },
	{ operation_type::inc_at, {0x34}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },
	{ operation_type::dec_at, {0x35}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	indexed_implied(operation_type::add, 0x86),
	indexed_implied(operation_type::adc, 0x8E),
	indexed_implied(operation_type::sub, 0x96),
	indexed_implied(operation_type::sbc, 0x9E),
	indexed_implied(operation_type::and_8, 0xA6),
	indexed_implied(operation_type::or_8, 0xB6),
	indexed_implied(operation_type::xor_8, 0xAE),
	indexed_implied(operation_type::cp, 0xBE),
	indexed_indexed(operation_type::inc_at, 0x34),
	indexed_indexed(operation_type::dec_at, 0x35),

	ixhl_iyhl(operation_type::add, 0x84, addressing_mode::implied),
	ixhl_iyhl(operation_type::adc, 0x8C, addressing_mode::implied),
	ixhl_iyhl(operation_type::sub, 0x94, addressing_mode::implied),
	ixhl_iyhl(operation_type::sbc, 0x9C, addressing_mode::implied),
	ixhl_iyhl(operation_type::and_8, 0xA4, addressing_mode::implied),
	ixhl_iyhl(operation_type::xor_8, 0xAC, addressing_mode::implied),
	ixhl_iyhl(operation_type::or_8, 0xB4, addressing_mode::implied),
	ixhl_iyhl(operation_type::cp, 0xBC, addressing_mode::implied),

	{ operation_type::inc_reg, {0xDD, 0x24}, addressing_mode::register_ixh, addressing_mode::register_ixh },
	{ operation_type::dec_reg, {0xDD, 0x25}, addressing_mode::register_ixh, addressing_mode::register_ixh },
	{ operation_type::inc_reg, {0xDD, 0x2C}, addressing_mode::register_ixl, addressing_mode::register_ixl },
	{ operation_type::dec_reg, {0xDD, 0x2D}, addressing_mode::register_ixl, addressing_mode::register_ixl },

	{ operation_type::inc_reg, {0xFD, 0x24}, addressing_mode::register_iyh, addressing_mode::register_iyh },
	{ operation_type::dec_reg, {0xFD, 0x25}, addressing_mode::register_iyh, addressing_mode::register_iyh },
	{ operation_type::inc_reg, {0xFD, 0x2C}, addressing_mode::register_iyl, addressing_mode::register_iyl },
	{ operation_type::dec_reg, {0xFD, 0x2D}, addressing_mode::register_iyl, addressing_mode::register_iyl },


	/* 16-Bit Arithmetic Group */
	{ operation_type::add_16, {0x09}, addressing_mode::register_bc, addressing_mode::register_hl },
	{ operation_type::add_16, {0x19}, addressing_mode::register_de, addressing_mode::register_hl },
	{ operation_type::add_16, {0x29}, addressing_mode::register_hl, addressing_mode::register_hl },
	{ operation_type::add_16, {0x39}, addressing_mode::register_sp, addressing_mode::register_hl },

	{ operation_type::add_16, {0xDD, 0x09}, addressing_mode::register_bc, addressing_mode::register_ix },
	{ operation_type::add_16, {0xDD, 0x19}, addressing_mode::register_de, addressing_mode::register_ix },
	{ operation_type::add_16, {0xDD, 0x29}, addressing_mode::register_ix, addressing_mode::register_ix },
	{ operation_type::add_16, {0xDD, 0x39}, addressing_mode::register_sp, addressing_mode::register_ix },

	{ operation_type::add_16, {0xFD, 0x09}, addressing_mode::register_bc, addressing_mode::register_iy },
	{ operation_type::add_16, {0xFD, 0x19}, addressing_mode::register_de, addressing_mode::register_iy },
	{ operation_type::add_16, {0xFD, 0x29}, addressing_mode::register_iy, addressing_mode::register_iy },
	{ operation_type::add_16, {0xFD, 0x39}, addressing_mode::register_sp, addressing_mode::register_iy },

	{ operation_type::adc_hl, {0xED, 0x4A}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x5A}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x6A}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x7A}, addressing_mode::register_sp, addressing_mode::implied },

	{ operation_type::sbc_hl, {0xED, 0x42}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x52}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x62}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x72}, addressing_mode::register_sp, addressing_mode::implied },

	ss_ss(operation_type::inc_reg_16, 0b00000011, 4),
	{ operation_type::inc_reg_16, {0xDD, 0x23}, addressing_mode::register_ix, addressing_mode::register_ix },
	{ operation_type::inc_reg_16, {0xFD, 0x23}, addressing_mode::register_iy, addressing_mode::register_iy },
	ss_ss(operation_type::dec_reg_16, 0b00001011, 4),
	{ operation_type::dec_reg_16, {0xDD, 0x2B}, addressing_mode::register_ix, addressing_mode::register_ix },
	{ operation_type::dec_reg_16, {0xFD, 0x2B}, addressing_mode::register_iy, addressing_mode::register_iy },

	/* 8-Bit Load Group */
	register_register(operation_type::ld_reg, 0b01000000),
	indexed_register(operation_type::ld_reg, 0b01000110),
	indirect_hl_register(operation_type::ld_reg, 0b01000110),
	immediate_register(operation_type::ld_reg, 0b00000110, 3),
	{ operation_type::ld_reg, {0x0A}, addressing_mode::indirect_bc, addressing_mode::register_a },
	{ operation_type::ld_reg, {0x1A}, addressing_mode::indirect_de, addressing_mode::register_a },
	{ operation_type::ld_reg, {0x3A}, addressing_mode::immediate_ext, addressing_mode::register_a },
	{ operation_type::ld_reg, {0xED, 0x47}, addressing_mode::register_a, addressing_mode::register_i },
	{ operation_type::ld_reg, {0xED, 0x4F}, addressing_mode::register_a, addressing_mode::register_r },

	{ operation_type::ld_reg, {0xDD, 0x26}, addressing_mode::immediate, addressing_mode::register_ixh },
	{ operation_type::ld_reg, {0xFD, 0x26}, addressing_mode::immediate, addressing_mode::register_iyh },
	{ operation_type::ld_reg, {0xDD, 0x2E}, addressing_mode::immediate, addressing_mode::register_ixl },
	{ operation_type::ld_reg, {0xFD, 0x2E}, addressing_mode::immediate, addressing_mode::register_iyl },

	ld_reg_reg(operation_type::ld_reg, 0x40, addressing_mode::register_b),
	ld_reg_reg(operation_type::ld_reg, 0x48, addressing_mode::register_c),
	ld_reg_reg(operation_type::ld_reg, 0x50, addressing_mode::register_d),
	ld_reg_reg(operation_type::ld_reg, 0x58, addressing_mode::register_e),
	ld_reg_reg(operation_type::ld_reg, 0x78, addressing_mode::register_a),

	ld_dd_reg_reg(operation_type::ld_reg, 0x60, addressing_mode::register_ixh),
	ld_dd_reg_reg(operation_type::ld_reg, 0x68, addressing_mode::register_ixl),
	ld_fd_reg_reg(operation_type::ld_reg, 0x60, addressing_mode::register_iyh),
	ld_fd_reg_reg(operation_type::ld_reg, 0x68, addressing_mode::register_iyl),

	register_indirect_hl(operation_type::ld_at, 0b01110000, 0),
	register_indexed(operation_type::ld_at, 0b01110000, 0),
	immediate_indexed(operation_type::ld_at, 0x36),
	{ operation_type::ld_at, {0x36}, addressing_mode::immediate, addressing_mode::indirect_hl },
	{ operation_type::ld_at, {0x02}, addressing_mode::register_a, addressing_mode::indirect_bc },
	{ operation_type::ld_at, {0x12}, addressing_mode::register_a, addressing_mode::indirect_de },
	{ operation_type::ld_at, {0x32}, addressing_mode::register_a, addressing_mode::immediate_ext },

	{ operation_type::ld_ir, {0xED, 0x57}, addressing_mode::register_i, addressing_mode::implied },
	{ operation_type::ld_ir, {0xED, 0x5F}, addressing_mode::register_r, addressing_mode::implied },

	/* 16-Bit Load Group */
	indirect_dd_des(operation_type::ld_16_reg, 0b00000001, 4, addressing_mode::immediate_ext),
	{ operation_type::ld_16_reg, {0xF9}, addressing_mode::register_hl, addressing_mode::register_sp },
	{ operation_type::ld_16_reg, {0xDD, 0xF9}, addressing_mode::register_ix, addressing_mode::register_sp },
	{ operation_type::ld_16_reg, {0xFD, 0xF9}, addressing_mode::register_iy, addressing_mode::register_sp },
	{ operation_type::ld_16_reg, {0xDD, 0x21}, addressing_mode::immediate_ext, addressing_mode::register_ix },
	{ operation_type::ld_16_reg, {0xFD, 0x21}, addressing_mode::immediate_ext, addressing_mode::register_iy },

	{ operation_type::ld_16_at, {0xED, 0x43}, addressing_mode::register_bc, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x53}, addressing_mode::register_de, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x63}, addressing_mode::register_hl, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x73}, addressing_mode::register_sp, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0x22}, addressing_mode::register_hl, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xDD, 0x22}, addressing_mode::register_ix, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xFD, 0x22}, addressing_mode::register_iy, addressing_mode::immediate_ext},

	{ operation_type::ld_16_reg_from, {0xED, 0x4B}, addressing_mode::immediate_ext, addressing_mode::register_bc },
	{ operation_type::ld_16_reg_from, {0xED, 0x5B}, addressing_mode::immediate_ext, addressing_mode::register_de },
	{ operation_type::ld_16_reg_from, {0xED, 0x6B}, addressing_mode::immediate_ext, addressing_mode::register_hl },
	{ operation_type::ld_16_reg_from, {0xED, 0x7B}, addressing_mode::immediate_ext, addressing_mode::register_sp },
	{ operation_type::ld_16_reg_from, {0x2A}, addressing_mode::immediate_ext, addressing_mode::register_hl },
	{ operation_type::ld_16_reg_from, {0xDD, 0x2A}, addressing_mode::immediate_ext, addressing_mode::register_ix },
	{ operation_type::ld_16_reg_from, {0xFD, 0x2A}, addressing_mode::immediate_ext, addressing_mode::register_iy },

	{ operation_type::push, {0xC5}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::push, {0xD5}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::push, {0xE5}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::push, {0xF5}, addressing_mode::register_af, addressing_mode::implied },
	{ operation_type::push, {0xDD, 0xE5}, addressing_mode::register_ix, addressing_mode::implied },
	{ operation_type::push, {0xFD, 0xE5}, addressing_mode::register_iy, addressing_mode::implied },

	{ operation_type::pop, {0xC1}, addressing_mode::implied, addressing_mode::register_bc },
	{ operation_type::pop, {0xD1}, addressing_mode::implied, addressing_mode::register_de },
	{ operation_type::pop, {0xE1}, addressing_mode::implied, addressing_mode::register_hl },
	{ operation_type::pop, {0xF1}, addressing_mode::implied, addressing_mode::register_af },
	{ operation_type::pop, {0xDD, 0xE1}, addressing_mode::implied, addressing_mode::register_ix },
	{ operation_type::pop, {0xFD, 0xE1}, addressing_mode::implied, addressing_mode::register_iy },

	/* Call and Return Group */
	{ operation_type::call, {0xCD}, addressing_mode::immediate_ext, addressing_mode::none },
	cc_inst(operation_type::call_cc, 0b11000100, addressing_mode::immediate_ext, addressing_mode::none),
	cc_inst(operation_type::rst, 0b11000111, addressing_mode::none, addressing_mode::none),
	{ operation_type::ret, {0xC9}, addressing_mode::none, addressing_mode::none },
	cc_inst(operation_type::ret_cc, 0b11000000, addressing_mode::none, addressing_mode::none),

	/* Jump Group */
	{ operation_type::jp, {0xC3}, addressing_mode::immediate_ext, addressing_mode::none },
	{ operation_type::jp, {0xE9}, addressing_mode::indirect_hl, addressing_mode::none },
	cc_inst(operation_type::jp_cc, 0b11000010, addressing_mode::immediate_ext, addressing_mode::none),
	{ operation_type::jr_z, {0x28}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::jr_nz, {0x20}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::jr_c, {0x38}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::jr_nc, {0x30}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::jr, {0x18}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::djnz, {0x10}, addressing_mode::immediate, addressing_mode::none },

	/* CPU Control Groups */
	{ operation_type::nop, {0x00}, addressing_mode::none, addressing_mode::none },
	{ operation_type::halt, {0x76}, addressing_mode::none, addressing_mode::none },
	{ operation_type::di, {0xF3}, addressing_mode::none, addressing_mode::none },
	{ operation_type::ei, {0xFB}, addressing_mode::none, addressing_mode::none },
	{ operation_type::im0, {0xED, 0x46}, addressing_mode::none, addressing_mode::none },
	{ operation_type::im1, {0xED, 0x56}, addressing_mode::none, addressing_mode::none },
	{ operation_type::im2, {0xED, 0x5E}, addressing_mode::none, addressing_mode::none },

	/* Input and Output Group */
	{ operation_type::out, {0xD3}, addressing_mode::immediate, addressing_mode::none },

	/* Exchange, Block Transfer, and Search Group */
	{ operation_type::ex_de_hl, {0xEB}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ex_af_afs, {0x08}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::exx, {0xD9}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ex_16_at, {0xE3}, addressing_mode::register_hl, addressing_mode::indirect_sp },
	{ operation_type::ex_16_at, {0xDD, 0xE3}, addressing_mode::register_ix, addressing_mode::indirect_sp },
	{ operation_type::ex_16_at, {0xFD, 0xE3}, addressing_mode::register_iy, addressing_mode::indirect_sp },
	{ operation_type::ldi, {0xED, 0xA0}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ldir, {0xED, 0xB0}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::cpd, {0xED, 0xA9}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::cpdr, {0xED, 0xB9}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::cpi, {0xED, 0xA1}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::cpir, {0xED, 0xB1}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ldd, {0xED, 0xA8}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::lddr, {0xED, 0xB8}, addressing_mode::implied, addressing_mode::implied },

	/* Rotate and Shift Group */
	{ operation_type::rlca, {0x07},	addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rrca, {0x0F},	addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rra, {0x1F}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rla, {0x17}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rld, {0xED, 0x6F}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rrd, {0xED, 0x67}, addressing_mode::implied, addressing_mode::implied },

	destination_register_wide(operation_type::rlc, 0xCB, 0x0, 0, addressing_mode::implied),
	{ operation_type::rlc_at, {0xCB, 0x06}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::rl, 0xCB, 0b00010000, 0, addressing_mode::implied),
	{ operation_type::rl_at, {0xCB, 0x16}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::rrc, 0xCB, 0b00001000, 0, addressing_mode::implied),
	{ operation_type::rrc_at, {0xCB, 0x0E}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::rr, 0xCB, 0b00011000, 0, addressing_mode::implied),
	{ operation_type::rr_at, {0xCB, 0x1E}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::sla, 0xCB, 0b00100000, 0, addressing_mode::implied),
	{ operation_type::sla_at, {0xCB, 0x26}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::sra, 0xCB, 0b00101000, 0, addressing_mode::implied),
	{ operation_type::sra_at, {0xCB, 0x2E}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::srl, 0xCB, 0b00111000, 0, addressing_mode::implied),
	{ operation_type::srl_at, {0xCB, 0x3E}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	destination_register_wide(operation_type::sll, 0xCB, 0b00110000, 0, addressing_mode::implied),
	{ operation_type::sll_at, {0xCB, 0x36}, addressing_mode::indirect_hl, addressing_mode::indirect_hl },

	/* Bit Set, Reset, and Test Group */
	bit_register(operation_type::tst_bit, 0xCB, 0b01000000),
	source_bit(operation_type::tst_bit_at, 0xCB, 0b01000110, addressing_mode::indirect_hl),

	bit_register(operation_type::set_bit, 0xCB, 0b11000000),
	source_bit(operation_type::set_bit_at, 0xCB, 0b11000110, addressing_mode::indirect_hl),

	bit_register(operation_type::res_bit, 0xCB, 0b10000000),
	source_bit(operation_type::res_bit_at, 0xCB, 0b10000110, addressing_mode::indirect_hl),

	// 2 highest bits in immediate byte contains actual operation type, so group up such instructions
	{ operation_type::bit_group, {0xDD, 0xCB}, addressing_mode::immediate_bit, addressing_mode::indexed_ix },
	{ operation_type::bit_group, {0xFD, 0xCB}, addressing_mode::immediate_bit, addressing_mode::indexed_iy },

	/* General-Purpose Arithmetic */
	{ operation_type::daa, {0x27}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::cpl, {0x2F}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::neg, {0xED, 0x44}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ccf, {0x3F}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::scf, {0x37}, addressing_mode::implied, addressing_mode::implied },
};


void print_header()
{
	std::cout << "const instruction instructions[] = {" << std::endl;
}

void print_trailer()
{
	std::cout << "};" << std::endl;
}

void print_instruction(instruction inst)
{
	std::cout << "\t";
	std::cout << "{ " << operation_type_str(inst.op_type) << ", ";

	std::cout << "{" << su::hex_str(inst.opcodes[0]);
	if(long_instruction(inst.opcodes[0]))
		std::cout << ", " << su::hex_str(inst.opcodes[1]);
	std::cout << "}, ";

	std::cout << addressing_mode_str(inst.source) << ", ";
	std::cout << addressing_mode_str(inst.destination) << " }, " << std::endl;
}


// g++ -std=c++20 -I../../../ instructions_generator.cpp -o instructions_generator
int main()
{
	std::sort(std::begin(all_instructions), std::end(all_instructions), [](instruction a, instruction b)
	{
		if(a.op_type != b.op_type)
			return a.op_type < b.op_type;
		
		int width_a = a.opcodes[1] == 0 ? 1 : 2;
		int width_b = b.opcodes[1] == 0 ? 1 : 2;

		if(width_a != width_b)
			return width_a < width_b;

		if(a.source != b.source)
			return a.source < b.source;
		
		return a.destination < b.destination;

		// std::uint8_t o1 = a.opcodes[0];
		// std::uint8_t o2 = b.opcodes[0];
		// if(o1 == o2)
		// {
		// 	o1 = a.opcodes[1];
		// 	o2 = b.opcodes[1];
		// }
		
		// return o1 < o2;
	});


	print_header();

	for(auto inst : all_instructions)
		print_instruction(inst);

	print_trailer();

	return EXIT_SUCCESS;
}
