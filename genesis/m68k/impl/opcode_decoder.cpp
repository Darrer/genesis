#include "opcode_decoder.h"
#include "instruction_ea_modes.hpp"

#include <array>
#include <algorithm>
#include <string>
#include <bit>

namespace genesis::m68k
{

enum class token
{
	one,
	zero,
	size,
	ea_mode,
	any, // underscoer - either one or zero
	unknown,
	end, // end of string
};

class tokenizer
{
public:
	tokenizer() = delete;

	constexpr static token next(std::string_view inst_template, std::uint8_t& pos)
	{
		if(pos >= inst_template.size())
			return token::end;

		const std::pair<const char*, token> token_map[] = {
			{ "1", token::one },
			{ "0", token::zero },
			{ "_", token::any },
			{ "sz", token::size },
			{ "<-ea->", token::ea_mode }
		};

		for(auto [str, token] : token_map)
		{
			if(has_substring_at(inst_template, str, pos))
			{
				pos += (std::uint8_t)std::string_view(str).size();
				return token;
			}
		}

		return token::unknown;
	}

private:
	constexpr static bool has_substring_at(std::string_view str, std::string_view substr, std::uint8_t pos)
	{
		return str.find(substr, pos) == pos;
	}
};

constexpr bool validate_opcodes()
{
	for(auto it = std::begin(opcodes); it != std::end(opcodes); ++it)
	{
		auto entry = *it;

		if(entry.inst_template.size() != 16)
			return false;

		std::uint8_t pos = 0;
		while(true)
		{
			auto token = tokenizer::next(entry.inst_template, pos);
			if(token == token::unknown)
				return false;
			if(token == token::end)
				break;
		}

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

constexpr const std::array<std::uint8_t, 3> size_bits = { 0b00, 0b01, 0b10 };

constexpr const std::array<std::uint8_t, 12> ea_all_bits = {
	0b000, 0b001, 0b010, 0b011, 0b100, 0b101, 0b110,
	0b111000, 0b111001, 0b111010, 0b111011, 0b111100
};

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
