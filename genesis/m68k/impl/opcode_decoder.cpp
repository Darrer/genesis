#include "opcode_decoder.h"
#include "instruction_ea_modes.hpp"

#include <array>
#include <algorithm>
#include <string>
#include <bit>

namespace genesis::m68k
{

constexpr bool validate_opcodes()
{
	auto valid_tokens = {'1', '0', '_', 's', 'z'};

	for(auto it = std::begin(opcodes); it != std::end(opcodes); ++it)
	{
		auto entry = *it;

		if(entry.inst_template.size() != 16)
			return false;

		// for(auto ch : entry.inst_template)
		// {
		// 	if(std::ranges::find(valid_tokens, ch) == valid_tokens.end())
		// 		return false;
		// }

		// check for duplicates
		for(auto next = std::next(it); next != std::end(opcodes); ++next)
		{
			auto next_entry = *next;
			if(entry.inst_template == next_entry.inst_template)
				return false;
		}
	}

	return true;
}

static_assert(validate_opcodes());


constexpr static bool has_sz_token(std::string_view str)
{
	return str.find("sz") != std::string::npos;
}

class opcode_builder
{
public:
	opcode_builder() = delete;

	constexpr static auto build_opcode_map()
	{
		std::array<inst_type, 0xFFFF + 1> opcode_map;
		opcode_map.fill(inst_type::NONE);

		// for(auto inst : opcodes)
		// {
			
		// }

		return opcode_map;
	}

};


constexpr const auto opcode_map = opcode_builder::build_opcode_map();

m68k::inst_type opcode_decoder::decode(std::uint16_t opcode)
{
	return opcode_map.at(opcode);
}

}
