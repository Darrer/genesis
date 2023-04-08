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
	ADDX,
	SUB,
	SUBI,
	SUBQ,
	SUBA,
	SUBX,
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
	NEG,
	NEGX,
	NOT,
	NOP,
	MOVE,
	MOVEQ,
	MOVEA,
	MOVEM,
};

}

#endif // __M68K_INSTRUCTION_TYPE_H__
