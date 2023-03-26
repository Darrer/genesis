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

const constexpr mask_inst_pair opcodes[] =
{
	/* ADD */
	{ "1101____________", inst_type::ADD },
	{ "1101____11______", inst_type::ADDA },
	{ "00000110________", inst_type::ADDI },
	{ "0101___0________", inst_type::ADDQ },

	/* SUB */
	{ "1001____________", inst_type::SUB },
	{ "1001____11______", inst_type::SUBA },
	{ "00000100________", inst_type::SUBI },
	{ "0101___1________", inst_type::SUBQ },

	/* AND */
	{ "1100____________", inst_type::AND },
	{ "00000010________", inst_type::ANDI },

	/* OR */
	{ "1000____________", inst_type::OR },
	{ "00000000________", inst_type::ORI },

	/* EOR */
	{ "1011___1________", inst_type::EOR },
	{ "00001010________", inst_type::EORI },

	/* CMP */
	{ "1011___0________", inst_type::CMP },
	{ "1011___1__001___", inst_type::CMPM },
	{ "00001100________", inst_type::CMPI },
};


class opcode_decoder
{
public:
	opcode_decoder() = delete;

	static m68k::inst_type decode(std::uint16_t opcode);
};

}

#endif // __M68K_OPCODE_DECODER_H__
