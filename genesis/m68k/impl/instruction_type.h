#ifndef __M68K_INSTRUCTION_TYPE_H__
#define __M68K_INSTRUCTION_TYPE_H__

#include <cstdint>


namespace genesis::m68k
{

enum inst_type : std::uint8_t
{
	NONE,
	ADD,
	ADDI,
	ADDQ,
	ADDA,
	SUB,
	SUBI,
	SUBQ,
	SUBA,
	AND,
	ANDI,
	OR,
	ORI,
	EOR,
	EORI,
	CMP,
	CMPI,
	CMPM,
	CMPA,
};

}

#endif // __M68K_INSTRUCTION_TYPE_H__
