#ifndef __INSTRUCTIONS_HPP__
#define __INSTRUCTIONS_HPP__

#include "z80/cpu.h"

#include <cstdint>
#include <array>


namespace genesis::z80
{

enum operation_type : std::uint8_t
{
	/* 8-Bit Arithmetic Group */
	add,
	adc,
	sub,
	sbc,
	and_8,
	or_8,
	xor_8,
	cp,
	inc_reg,
	inc_at,
	dec_reg,
	dec_at,

	/* 16-Bit Arithmetic Group */
	add_hl,
	adc_hl,
	sbc_hl,
	inc_reg_16,
	dec_reg_16,

	/* 8/16-Bit Load Group */
	ld_reg,
	ld_at,
	ld_ir,
	ld_16_reg,
	ld_16_reg_from,
	ld_16_at,
	push,
	pop,

	/* Call and Return Group */
	call,
	call_cc,
	ret,
	ret_cc,

	/* Jump Group */
	jp,
	jp_cc,
	jr_z,
	jr,

	/* CPU Control Groups */
	di,
	ei,
	nop,

	/* Input and Output Group */
	out,

	/* Exchange, Block Transfer, and Search Group */
	ex_de_hl,
	ex_af_afs,
	exx,
	ldir,

	/* Rotate and Shift Group */
	rlca,
	rrca,
};

enum addressing_mode : std::uint8_t
{
	none,

	implied,

	// register
	register_a,
	register_b,
	register_c,
	register_d,
	register_e,
	register_h,
	register_l,

	register_i,
	register_r,

	// register pair
	register_af,
	register_bc,
	register_de,
	register_hl,
	register_sp,
	register_ix,
	register_iy,

	// immediate
	immediate,
	immediate_ext,

	// indirect
	indirect_hl,
	indirect_bc,
	indirect_de,
	indirect_sp, // do we need it?
	indirect_af, // do we need it?

	// indexed
	indexed_ix,
	indexed_iy
};

class instruction
{
public:
	z80::operation_type op_type;
	std::array<z80::opcode, 2> opcodes = {0x0, 0x0};

	addressing_mode source;
	addressing_mode destination;
};


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

const instruction instructions[] = {
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

	/* 16-Bit Arithmetic Group */
	{ operation_type::add_hl, {0x09}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::add_hl, {0x19}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::add_hl, {0x29}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::add_hl, {0x39}, addressing_mode::register_sp, addressing_mode::implied },

	{ operation_type::adc_hl, {0xED, 0x4A}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x5A}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x6A}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::adc_hl, {0xED, 0x7A}, addressing_mode::register_sp, addressing_mode::implied },

	{ operation_type::sbc_hl, {0xED, 0x42}, addressing_mode::register_bc, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x52}, addressing_mode::register_de, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x62}, addressing_mode::register_hl, addressing_mode::implied },
	{ operation_type::sbc_hl, {0xED, 0x72}, addressing_mode::register_sp, addressing_mode::implied },

	ss_ss(operation_type::inc_reg_16, 0b00000011, 4),
	ss_ss(operation_type::dec_reg_16, 0b00001011, 4),

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

	register_indirect_hl(operation_type::ld_at, 0b01110000, 0),
	register_indexed(operation_type::ld_at, 0b01110000, 0),
	immediate_indexed(operation_type::ld_at, 0x36),
	{ operation_type::ld_at, {0x36}, addressing_mode::immediate, addressing_mode::indirect_hl },
	{ operation_type::ld_at, {0x02}, addressing_mode::register_a, addressing_mode::indirect_bc },
	{ operation_type::ld_at, {0x12}, addressing_mode::register_a, addressing_mode::indirect_de },
	{ operation_type::ld_at, {0x32}, addressing_mode::register_a, addressing_mode::immediate_ext },

	{ operation_type::ld_16_at, {0xED, 0x43}, addressing_mode::register_bc, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x53}, addressing_mode::register_de, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x63}, addressing_mode::register_hl, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0xED, 0x73}, addressing_mode::register_sp, addressing_mode::immediate_ext},
	{ operation_type::ld_16_at, {0x22}, addressing_mode::register_hl, addressing_mode::immediate_ext},

	{ operation_type::ld_ir, {0xED, 0x57}, addressing_mode::register_i, addressing_mode::implied },
	{ operation_type::ld_ir, {0xED, 0x5F}, addressing_mode::register_r, addressing_mode::implied },

	/* 16-Bit Load Group */
	indirect_dd_des(operation_type::ld_16_reg, 0b00000001, 4, addressing_mode::immediate_ext),
	{ operation_type::ld_16_reg, {0xF9}, addressing_mode::register_hl, addressing_mode::register_sp },
	{ operation_type::ld_16_reg, {0xFD, 0x21}, addressing_mode::immediate_ext, addressing_mode::register_iy },

	{ operation_type::ld_16_reg_from, {0xED, 0x4B}, addressing_mode::immediate_ext, addressing_mode::register_bc },
	{ operation_type::ld_16_reg_from, {0xED, 0x5B}, addressing_mode::immediate_ext, addressing_mode::register_de },
	{ operation_type::ld_16_reg_from, {0xED, 0x6B}, addressing_mode::immediate_ext, addressing_mode::register_hl },
	{ operation_type::ld_16_reg_from, {0xED, 0x7B}, addressing_mode::immediate_ext, addressing_mode::register_sp },
	{ operation_type::ld_16_reg_from, {0x2A}, addressing_mode::immediate_ext, addressing_mode::register_hl },

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
	{ operation_type::pop, {0xFD, 0xE1}, addressing_mode::implied, addressing_mode::register_iy },
	{ operation_type::pop, {0xDD, 0xE1}, addressing_mode::implied, addressing_mode::register_ix },

	/* Call and Return Group */
	{ operation_type::call, {0xCD}, addressing_mode::immediate_ext, addressing_mode::none },
	{ operation_type::ret, {0xC9}, addressing_mode::implied, addressing_mode::none },
	cc_inst(operation_type::call_cc, 0b11000100, addressing_mode::immediate_ext, addressing_mode::none),
	cc_inst(operation_type::ret_cc, 0b11000000, addressing_mode::implied, addressing_mode::none),

	/* Jump Group */
	{ operation_type::jp, {0xC3}, addressing_mode::immediate_ext, addressing_mode::none },
	cc_inst(operation_type::jp_cc, 0b11000010, addressing_mode::immediate_ext, addressing_mode::none),
	{ operation_type::jr_z, {0x28}, addressing_mode::immediate, addressing_mode::none },
	{ operation_type::jr, {0x18}, addressing_mode::immediate, addressing_mode::none },

	/* CPU Control Groups */
	{ operation_type::di, {0xF3}, addressing_mode::none, addressing_mode::none },
	{ operation_type::ei, {0xFB}, addressing_mode::none, addressing_mode::none },
	{ operation_type::nop, {0x00}, addressing_mode::none, addressing_mode::none },

	/* Input and Output Group */
	{ operation_type::out, {0xD3}, addressing_mode::immediate, addressing_mode::none },

	/* Exchange, Block Transfer, and Search Group */
	{ operation_type::ex_de_hl, {0xEB}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ex_af_afs, {0x08}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::exx, {0xD9}, addressing_mode::implied, addressing_mode::implied },
	{ operation_type::ldir, {0xED, 0xB0}, addressing_mode::implied, addressing_mode::implied },

	/* Rotate and Shift Group */
	{ operation_type::rlca, {0x07},	addressing_mode::implied, addressing_mode::implied },
	{ operation_type::rrca, {0x0F},	addressing_mode::implied, addressing_mode::implied },
};

}

#endif // __INSTRUCTIONS_HPP__
