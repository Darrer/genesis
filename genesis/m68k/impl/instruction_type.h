#ifndef __M68K_INSTRUCTION_TYPE_H__
#define __M68K_INSTRUCTION_TYPE_H__


namespace genesis::m68k
{

enum class inst_type
{
	NONE,
	ADD,
	ADDI,
	ADDQ,
	ADDA,
	ADDX,
	ANDItoCCR,
	ANDItoSR,
	SUB,
	SUBI,
	SUBQ,
	SUBA,
	SUBX,
	AND,
	ANDI,
	OR,
	ORI,
	ORItoCCR,
	ORItoSR,
	EOR,
	EORI,
	EORItoCCR,
	EORItoSR,
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
	MOVEP,
	MOVEfromSR,
	MOVEtoSR,
	MOVE_USP,
	MOVEtoCCR,
	ASLRreg,
	ASLRmem,
	ROLRreg,
	ROLRmem,
	LSLRreg,
	LSLRmem,
	ROXLRreg,
	ROXLRmem,
	TST,
	CLR,
	MULU,
	MULS,
	TRAP,
	TRAPV,
	DIVU,
	DIVS,
	EXT,
	EXG,
};

}

#endif // __M68K_INSTRUCTION_TYPE_H__
