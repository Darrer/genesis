#ifndef __DISPATCHING_TABLES_HPP__
#define __DISPATCHING_TABLES_HPP__

#include <cstdint>
#include "executioner.hpp"

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

struct register_operation
{
	z80::operation_type op_type;
	z80::opcode opcode;
	z80::register_type reg;
};

#define register_op(op, opcode, reg_offset) \
	{op, opcode | (register_type::A << reg_offset), register_type::A}, \
	{op, opcode | (register_type::B << reg_offset), register_type::B}, \
	{op, opcode | (register_type::C << reg_offset), register_type::C}, \
	{op, opcode | (register_type::D << reg_offset), register_type::D}, \
	{op, opcode | (register_type::E << reg_offset), register_type::E}, \
	{op, opcode | (register_type::H << reg_offset), register_type::H}, \
	{op, opcode | (register_type::L << reg_offset), register_type::L}

/* {OP} r */
register_operation register_ops[] = {
	register_op(operation_type::add, 0b10000000, 0),
	register_op(operation_type::adc, 0b10001000, 0),
	register_op(operation_type::sub, 0b10010000, 0),
	register_op(operation_type::sbc, 0b10011000, 0),
	register_op(operation_type::and_8, 0b10100000, 0),
	register_op(operation_type::or_8, 0b10110000, 0),
	register_op(operation_type::xor_8, 0b10101000, 0),
	register_op(operation_type::cp, 0b10111000, 0),
	register_op(operation_type::inc_reg, 0b00000100, 3),
	register_op(operation_type::dec_reg, 0b00000101, 3),
};


struct immediate_operation
{
	z80::operation_type op_type;
	std::uint8_t opcode;
};

/* {OP} n */
immediate_operation immediate_ops[] = {
	{ operation_type::add, 0xC6 },
	{ operation_type::adc, 0xCE },
	{ operation_type::sub, 0xD6 },
	{ operation_type::sbc, 0xDE },
	{ operation_type::and_8, 0xE6 },
	{ operation_type::or_8, 0xF6 },
	{ operation_type::xor_8, 0xEE },
	{ operation_type::cp, 0xFE },
};

enum access
{
	read,
	write,
};

struct indirect_operation
{
	z80::operation_type op_type;
	std::uint8_t opcode;
	z80::access access_type = access::read;
};

/* {OP} (HL) */
indirect_operation indirect_ops[] = {
	{ operation_type::add, 0x86 },
	{ operation_type::adc, 0x8E },
	{ operation_type::sub, 0x96 },
	{ operation_type::sbc, 0x9E },
	{ operation_type::and_8, 0xA6 },
	{ operation_type::or_8, 0xB6 },
	{ operation_type::xor_8, 0xAE },
	{ operation_type::cp, 0xBE },
	{ operation_type::inc_at, 0x34, access::write },
	{ operation_type::dec_at, 0x35, access::write },
};


struct indexed_operation
{
	z80::operation_type op_type;
	std::uint8_t opcode;
	std::uint8_t opcode2;
	z80::access access_type = access::read;
};

#define indexed_op(op, opcode2, ...) \
	{ op, 0xDD, opcode2, __VA_ARGS__ }, \
	{ op, 0xFD, opcode2, __VA_ARGS__ }

/* {OP} (IX+d) */
/* {OP} (IY+d) */
indexed_operation indexed_ops[] = {
	indexed_op(operation_type::add, 0x86),
	indexed_op(operation_type::adc, 0x8E),
	indexed_op(operation_type::sub, 0x96),
	indexed_op(operation_type::sbc, 0x9E),
	indexed_op(operation_type::and_8, 0xA6),
	indexed_op(operation_type::or_8, 0xB6),
	indexed_op(operation_type::xor_8, 0xAE),
	indexed_op(operation_type::cp, 0xBE),
	indexed_op(operation_type::inc_at, 0x34, access::write),
	indexed_op(operation_type::dec_at, 0x35, access::write),
};

} // genesis::z80

#endif // __DISPATCHING_TABLES_HPP__
