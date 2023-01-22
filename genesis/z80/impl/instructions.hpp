#ifndef __INSTRUCTIONS_HPP__
#define __INSTRUCTIONS_HPP__

#include "../cpu.h"

#include <cstdint>
#include <array>


namespace genesis::z80
{

enum operation_type : std::uint8_t
{
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
	ld_reg,
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

	immediate,

	// indirect
	indirect_hl,

	indexed_ix,
	indexed_iy
};

class instruction
{
public:
	z80::operation_type op_type;
	std::array<z80::opcode, 2> opcodes = {0x0, 0x0};
	// std::initializer_list<z80::opcode> opcodes;

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
#define register_implied(op, opcode, reg_offset) \
	{op, { opcode | (register_type::A << reg_offset) }, addressing_mode::register_a, addressing_mode::implied}, \
	{op, { opcode | (register_type::B << reg_offset) }, addressing_mode::register_b, addressing_mode::implied}, \
	{op, { opcode | (register_type::C << reg_offset) }, addressing_mode::register_c, addressing_mode::implied}, \
	{op, { opcode | (register_type::D << reg_offset) }, addressing_mode::register_d, addressing_mode::implied}, \
	{op, { opcode | (register_type::E << reg_offset) }, addressing_mode::register_e, addressing_mode::implied}, \
	{op, { opcode | (register_type::H << reg_offset) }, addressing_mode::register_h, addressing_mode::implied}, \
	{op, { opcode | (register_type::L << reg_offset) }, addressing_mode::register_l, addressing_mode::implied}

#define register_register_impl(op, opcode, dest) \
	{op, { opcode | register_type::A }, addressing_mode::register_a, dest}, \
	{op, { opcode | register_type::B }, addressing_mode::register_b, dest}, \
	{op, { opcode | register_type::C }, addressing_mode::register_c, dest}, \
	{op, { opcode | register_type::D }, addressing_mode::register_d, dest}, \
	{op, { opcode | register_type::E }, addressing_mode::register_e, dest}, \
	{op, { opcode | register_type::H }, addressing_mode::register_h, dest}, \
	{op, { opcode | register_type::L }, addressing_mode::register_l, dest}

#define register_register(op, opcode) \
	register_register_impl(op, opcode | (register_type::A << 3), addressing_mode::register_a), \
	register_register_impl(op, opcode | (register_type::B << 3), addressing_mode::register_b), \
	register_register_impl(op, opcode | (register_type::C << 3), addressing_mode::register_c), \
	register_register_impl(op, opcode | (register_type::D << 3), addressing_mode::register_d), \
	register_register_impl(op, opcode | (register_type::E << 3), addressing_mode::register_e), \
	register_register_impl(op, opcode | (register_type::H << 3), addressing_mode::register_h), \
	register_register_impl(op, opcode | (register_type::L << 3), addressing_mode::register_l)

#define immediate_register(op, opcode) \
	{op, {opcode | (register_type::A << 3)}, addressing_mode::immediate, addressing_mode::register_a}, \
	{op, {opcode | (register_type::B << 3)}, addressing_mode::immediate, addressing_mode::register_b}, \
	{op, {opcode | (register_type::C << 3)}, addressing_mode::immediate, addressing_mode::register_c}, \
	{op, {opcode | (register_type::D << 3)}, addressing_mode::immediate, addressing_mode::register_d}, \
	{op, {opcode | (register_type::E << 3)}, addressing_mode::immediate, addressing_mode::register_e}, \
	{op, {opcode | (register_type::H << 3)}, addressing_mode::immediate, addressing_mode::register_h}, \
	{op, {opcode | (register_type::L << 3)}, addressing_mode::immediate, addressing_mode::register_l}


#define immediate_implied(op, opcode) \
	{op, {opcode}, addressing_mode::immediate, addressing_mode::implied}

#define indirect_hl_implied(op, opcode) \
	{op, {opcode}, addressing_mode::indirect_hl, addressing_mode::implied}

#define indirect_indirect(op, opcode, src_dest) \
	{op, {opcode}, src_dest, src_dest}

#define indexed_implied(op, opcode2) \
	{op, {0xDD, opcode2}, addressing_mode::indexed_ix, addressing_mode::implied}, \
	{op, {0xFD, opcode2}, addressing_mode::indexed_iy, addressing_mode::implied}

#define indexed_indexed(op, opcode2) \
	{op, {0xDD, opcode2}, addressing_mode::indexed_ix, addressing_mode::indexed_ix}, \
	{op, {0xFD, opcode2}, addressing_mode::indexed_iy, addressing_mode::indexed_iy}

const instruction instructionss[] = {
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

	register_register(operation_type::ld_reg, 0b01000000),

	immediate_implied(operation_type::add, 0xC6),
	immediate_implied(operation_type::adc, 0xCE),
	immediate_implied(operation_type::sub, 0xD6),
	immediate_implied(operation_type::sbc, 0xDE),
	immediate_implied(operation_type::and_8, 0xE6),
	immediate_implied(operation_type::or_8, 0xF6),
	immediate_implied(operation_type::xor_8, 0xEE),
	immediate_implied(operation_type::cp, 0xFE),

	immediate_register(operation_type::ld_reg, 0b00000110),

	indirect_hl_implied(operation_type::add, 0x86),
	indirect_hl_implied(operation_type::adc, 0x8E),
	indirect_hl_implied(operation_type::sub, 0x96),
	indirect_hl_implied(operation_type::sbc, 0x9E),
	indirect_hl_implied(operation_type::and_8, 0xA6),
	indirect_hl_implied(operation_type::or_8, 0xB6),
	indirect_hl_implied(operation_type::xor_8, 0xAE),
	indirect_hl_implied(operation_type::cp, 0xBE),
	indirect_indirect(operation_type::inc_at, 0x34, addressing_mode::indirect_hl),
	indirect_indirect(operation_type::dec_at, 0x35, addressing_mode::indirect_hl),

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
};

}

#endif // __INSTRUCTIONS_HPP__
