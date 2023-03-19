#ifndef __M68K_INSTRUCTION_TYPE_H__
#define __M68K_INSTRUCTION_TYPE_H__

namespace genesis::m68k
{

enum inst_type : std::uint8_t
{
	ADD,
	ADDI,
	ADDQ,
	SUB,
	SUBI,
	SUBQ,
	AND,
	ANDI,
	OR,
	ORI,
};

}

#endif // __M68K_INSTRUCTION_TYPE_H__
