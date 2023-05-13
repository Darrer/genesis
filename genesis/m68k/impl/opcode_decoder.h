#ifndef __M68K_OPCODE_DECODER_H__
#define __M68K_OPCODE_DECODER_H__

#include <string_view>
#include "instruction_type.h"
#include "ea_modes.h"

namespace genesis::m68k
{

using namespace genesis::m68k::impl;

struct instruction
{
	std::string_view inst_template;
	inst_type inst;
	ea_modes src_ea_mode = ea_modes::none;
	ea_modes dst_ea_mode = ea_modes::none; // MOVE instruction only
};

/* Placeholders:
	- 0/1 -- self-explanatory
	- _ -- either 1 or 0
	- sz -- any of 00 01 10
*/

const constexpr instruction opcodes[] =
{
	/* ADD */
	{ "1101___0sz<-ea->", inst_type::ADD,  ea_modes::all },
	{ "1101___1sz<-ea->", inst_type::ADD,  ea_modes::memory_alterable },
	{ "1101____11<-ea->", inst_type::ADDA, ea_modes::all },
	{ "00000110sz<-ea->", inst_type::ADDI, ea_modes::data_alterable },
	{ "0101___0sz<-ea->", inst_type::ADDQ, ea_modes::alterable },
	{ "1101___1sz00____", inst_type::ADDX },
	{ "0000001000111100", inst_type::ANDItoCCR },
	{ "0000001001111100", inst_type::ANDItoSR },

	/* SUB */
	{ "1001___0sz<-ea->", inst_type::SUB,  ea_modes::all },
	{ "1001___1sz<-ea->", inst_type::SUB,  ea_modes::memory_alterable },
	{ "1001____11<-ea->", inst_type::SUBA, ea_modes::all },
	{ "00000100sz<-ea->", inst_type::SUBI, ea_modes::data_alterable },
	{ "0101___1sz<-ea->", inst_type::SUBQ, ea_modes::alterable },
	{ "1001___1sz00____", inst_type::SUBX },

	/* AND */
	{ "1100___0sz<-ea->", inst_type::AND,  ea_modes::data },
	{ "1100___1sz<-ea->", inst_type::AND,  ea_modes::memory_alterable },
	{ "00000010sz<-ea->", inst_type::ANDI, ea_modes::data_alterable },

	/* OR */
	{ "1000___0sz<-ea->", inst_type::OR,  ea_modes::data },
	{ "1000___1sz<-ea->", inst_type::OR,  ea_modes::memory_alterable },
	{ "00000000sz<-ea->", inst_type::ORI, ea_modes::data_alterable },
	{ "0000000000111100", inst_type::ORItoCCR },
	{ "0000000001111100", inst_type::ORItoSR },

	/* EOR */
	{ "1011___1sz<-ea->", inst_type::EOR,  ea_modes::data_alterable },
	{ "00001010sz<-ea->", inst_type::EORI, ea_modes::data_alterable },
	{ "0000101000111100", inst_type::EORItoCCR },
	{ "0000101001111100", inst_type::EORItoSR },

	/* CMP */
	{ "1011___0sz<-ea->", inst_type::CMP,  ea_modes::all },
	{ "1011____11<-ea->", inst_type::CMPA, ea_modes::all },
	{ "00001100sz<-ea->", inst_type::CMPI, ea_modes::data_alterable },
	{ "1011___1sz001___", inst_type::CMPM },

	/* NEG */
	{ "01000100sz<-ea->", inst_type::NEG,  ea_modes::data_alterable },
	{ "01000000sz<-ea->", inst_type::NEGX, ea_modes::data_alterable },

	/* NOT */
	{ "01000110sz<-ea->", inst_type::NOT, ea_modes::data_alterable },

	/* NOP */
	{ "0100111001110001", inst_type::NOP },

	/* MOVE */
	{ "0001<-ea-><-ea->", inst_type::MOVE,  ea_modes::data, ea_modes::data_alterable },
	{ "0011<-ea-><-ea->", inst_type::MOVE,  ea_modes::all, ea_modes::data_alterable },
	{ "0010<-ea-><-ea->", inst_type::MOVE,  ea_modes::all, ea_modes::data_alterable },
	{ "001____001<-ea->", inst_type::MOVEA, ea_modes::all },
	{ "010010001_<-ea->", inst_type::MOVEMtoMEM, ea_modes::predecrement },
	{ "010011001_<-ea->", inst_type::MOVEMtoREG, ea_modes::postincrement },
	{ "0100000011<-ea->", inst_type::MOVEfromSR, ea_modes::data_alterable },
	{ "0100011011<-ea->", inst_type::MOVEtoSR, ea_modes::data },
	{ "0100010011<-ea->", inst_type::MOVEtoCCR, ea_modes::data },
	{ "010011100110____", inst_type::MOVE_USP },
	{ "0000___1__001___", inst_type::MOVEP },
	{ "0111___0________", inst_type::MOVEQ },

	/* ASL/ASR/ROL/ROR/LSL/LSR */
	{ "1110000_11<-ea->", inst_type::ASLRmem,  ea_modes::memory_alterable },
	{ "1110011_11<-ea->", inst_type::ROLRmem,  ea_modes::memory_alterable },
	{ "1110001_11<-ea->", inst_type::LSLRmem,  ea_modes::memory_alterable },
	{ "1110010_11<-ea->", inst_type::ROXLRmem, ea_modes::memory_alterable},
	{ "1110____sz_00___", inst_type::ASLRreg },
	{ "1110____sz_11___", inst_type::ROLRreg },
	{ "1110____sz_01___", inst_type::LSLRreg },
	{ "1110____sz_10___", inst_type::ROXLRreg },

	/* TST */
	{ "01001010sz<-ea->", inst_type::TST, ea_modes::data_alterable },

	/* CLR */
	{ "01000010sz<-ea->", inst_type::CLR, ea_modes::data_alterable },

	/* MULU/MULS */
	{ "1100___011<-ea->", inst_type::MULU, ea_modes::data },
	{ "1100___111<-ea->", inst_type::MULS, ea_modes::data },

	/* DIVU/DIVS */
	{ "1000___011<-ea->", inst_type::DIVU, ea_modes::data },
	{ "1000___111<-ea->", inst_type::DIVS, ea_modes::data },

	/* TRAP */
	{ "010011100100____", inst_type::TRAP },
	{ "0100111001110110", inst_type::TRAPV },


	/* EXT */
	{ "010010001_000___", inst_type::EXT },

	/* EXG */
	{ "1100___101000___", inst_type::EXG },
	{ "1100___101001___", inst_type::EXG },
	{ "1100___110001___", inst_type::EXG },

	/* SWAP */
	{ "0100100001000___", inst_type::SWAP },

	/* BIT */
	{ "0000___100<-ea->", inst_type::BTSTreg, ea_modes::data },
	{ "0000100000<-ea->", inst_type::BTSTimm, ea_modes::data_except_imm },

	{ "0000___111<-ea->", inst_type::BSETreg, ea_modes::data_alterable },
	{ "0000100011<-ea->", inst_type::BSETimm, ea_modes::data_alterable },

	{ "0000___110<-ea->", inst_type::BCLRreg, ea_modes::data_alterable },
	{ "0000100010<-ea->", inst_type::BCLRimm, ea_modes::data_alterable },

	{ "0000___101<-ea->", inst_type::BCHGreg, ea_modes::data_alterable },
	{ "0000100001<-ea->", inst_type::BCHGimm, ea_modes::data_alterable },

	/* RTE/RTR/RTS */
	{ "0100111001110011", inst_type::RTE },
	{ "0100111001110111", inst_type::RTR },
	{ "0100111001110101", inst_type::RTS },

	/* JMP */
	{ "0100111011<-ea->", inst_type::JMP, ea_modes::control },

	/* CHK */
	{ "0100___110<-ea->", inst_type::CHK, ea_modes::data },

	/* JSR/BSR */
	{ "0100111010<-ea->", inst_type::JSR, ea_modes::control },
	{ "01100001________", inst_type::BSR },

	/* LEA/PEA */
	{ "0100___111<-ea->", inst_type::LEA, ea_modes::control },
	{ "0100100001<-ea->", inst_type::PEA, ea_modes::control },

	/* LINK/UNLK */
	{ "0100111001010___", inst_type::LINK },
	{ "0100111001011___", inst_type::UNLK },

	/* BCC/DBCC/SCC */
	{ "0110____________", inst_type::BCC },
	// { "01100000________", inst_type::BRA }, // TODO: do we need it?
	{ "0101____11001___", inst_type::DBCC },
	{ "0101____11<-ea->", inst_type::SCC, ea_modes::data_alterable },

	/* ABCD/SBCD/NBCD */
	{ "1100___100000___", inst_type::ABCDreg },
	{ "1100___100001___", inst_type::ABCDmem },
	{ "1000___100000___", inst_type::SBCDreg },
	{ "1000___100001___", inst_type::SBCDmem },
	{ "0100100000<-ea->", inst_type::NBCD, ea_modes::data_alterable },

	/* RESET */
	{ "0100111001110000", inst_type::RESET },

	/* TAS */
	{ "0100101011<-ea->", inst_type::TAS, ea_modes::data_alterable },

	/* STOP */
	{ "0100111001110010", inst_type::STOP },

	{ "0100101011111100", inst_type::ILLEGAL },
};


class opcode_decoder
{
public:
	opcode_decoder() = delete;

	static m68k::inst_type decode(std::uint16_t opcode);
};

}

#endif // __M68K_OPCODE_DECODER_H__
