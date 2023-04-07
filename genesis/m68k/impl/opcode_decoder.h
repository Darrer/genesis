#ifndef __M68K_OPCODE_DECODER_H__
#define __M68K_OPCODE_DECODER_H__

#include <string_view>
#include "instruction_type.h"


namespace genesis::m68k
{

struct mask_inst_pair
{
	std::string_view inst_template;
	inst_type inst;
};

/* Placeholders:
	- 0/1 -- self-explanatory
	- _ -- either 1 or 0
	- sz -- any of 00 01 10
*/

const constexpr mask_inst_pair opcodes[] =
{
	/* ADD */
	{ "1101____sz______", inst_type::ADD },
	{ "1101____11______", inst_type::ADDA },
	{ "00000110sz______", inst_type::ADDI },
	{ "0101___0sz______", inst_type::ADDQ },
	{ "1101___1sz00____", inst_type::ADDX },

	/* SUB */
	{ "1001____sz______", inst_type::SUB },
	{ "1001____11______", inst_type::SUBA },
	{ "00000100sz______", inst_type::SUBI },
	{ "0101___1sz______", inst_type::SUBQ },
	{ "1001___1sz00____", inst_type::SUBX },

	/* AND */
	{ "1100____sz______", inst_type::AND },
	{ "00000010sz______", inst_type::ANDI },

	/* OR */
	{ "1000____sz______", inst_type::OR },
	{ "00000000sz______", inst_type::ORI },

	/* EOR */
	{ "1011___1sz______", inst_type::EOR },
	{ "00001010sz______", inst_type::EORI },

	/* CMP */
	{ "1011___0sz______", inst_type::CMP },
	{ "1011____11______", inst_type::CMPA },
	{ "1011___1sz001___", inst_type::CMPM },
	{ "00001100sz______", inst_type::CMPI },

	/* NEG */
	{ "01000100sz______", inst_type::NEG },
	{ "01000000sz______", inst_type::NEGX },

	/* NOT */
	{ "01000110sz______", inst_type::NOT },

	/* NOP */
	{ "0100111001110001", inst_type::NOP },

	/* MOVE */
	{ "0001____________", inst_type::MOVE },
	{ "0011____________", inst_type::MOVE },
	{ "0010____________", inst_type::MOVE },
	{ "0111___0________", inst_type::MOVEQ },
	{ "001____001______", inst_type::MOVEA },
};


class opcode_decoder
{
public:
	opcode_decoder() = delete;

	static m68k::inst_type decode(std::uint16_t opcode);
};

}

#endif // __M68K_OPCODE_DECODER_H__
