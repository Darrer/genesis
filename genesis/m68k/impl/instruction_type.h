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
	MOVEB,
	MOVE,
	MOVEQ,
	MOVEA,
	MOVEMtoREG,
	MOVEMtoMEM,
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
	SWAP,
	BTSTreg,
	BTSTimm,
	BSETreg,
	BSETimm,
	BCLRreg,
	BCLRimm,
	BCHGreg,
	BCHGimm,
	RTE,
	RTR,
	RTS,
	JMP,
	CHK,
	JSR,
	BSR,
	LEA,
	PEA,
	LINK,
	UNLK,
	BCC,
	DBCC,
	SCC,
	ABCDreg,
	ABCDmem,
	SBCDreg,
	SBCDmem,
	NBCD,
	RESET,
	TAS,
	STOP, // TODO: implement it
	ILLEGAL, // TODO: implement it
	BRA, // TODO: implement it
};

}

#endif // __M68K_INSTRUCTION_TYPE_H__
