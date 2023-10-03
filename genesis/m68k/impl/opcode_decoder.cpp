#include "opcode_decoder.h"

#include <algorithm>
#include <array>
#include <bit>
#include <iostream>
#include <string>

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
			{"1", token::one}, {"0", token::zero}, {"_", token::any}, {"sz", token::size}, {"<-ea->", token::ea_mode}};

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
		bool has_ea_mode = false;
		while(true)
		{
			auto token = tokenizer::next(entry.inst_template, pos);
			if(token == token::unknown)
				return false;
			if(token == token::end)
				break;
			if(token == token::ea_mode)
				has_ea_mode = true;
		}

		if(has_ea_mode && entry.src_ea_mode == ea_modes::none)
			return false;

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


class opcode_builder
{
public:
	opcode_builder() = delete;

	static auto build_opcode_map()
	{
		std::array<inst_type, 0xFFFF + 1> opcode_map;
		opcode_map.fill(inst_type::NONE);

		std::uint16_t opcode = 0;
		while(true)
		{
			for(auto inst : opcodes)
			{
				if(matches(opcode, inst))
				{
					opcode_map.at(opcode) = inst.inst;
					break;
				}
			}

			if(opcode == 0xFFFF)
				break;
			++opcode;
		}

		return opcode_map;
	}

private:
	constexpr static bool matches(std::uint16_t opcode, instruction inst)
	{
		bool first_ea_mode = true;
		std::uint8_t pos = 0;
		std::uint8_t size = 0xFF;
		while(true)
		{
			auto token = tokenizer::next(inst.inst_template, pos);
			if(token == token::end)
				return true;

			std::uint8_t bit_pos = 16 - pos;

			switch(token)
			{
			case token::one:
			case token::zero: {
				std::uint8_t val = (opcode >> bit_pos) & 1;
				if(token == token::one && val != 1)
					return false;
				if(token == token::zero && val != 0)
					return false;
				break;
			}

			case token::any:
				break;

			case token::size:
				if(!size_matches(opcode, bit_pos))
					return false;
				size = (opcode >> bit_pos) & 0b11;
				break;

			case token::ea_mode: {
				ea_modes modes;
				if(first_ea_mode)
				{
					first_ea_mode = false;
					modes = inst.dst_ea_mode != ea_modes::none ? inst.dst_ea_mode : inst.src_ea_mode;
				}
				else
				{
					modes = inst.src_ea_mode;
				}

				std::uint8_t ea = (opcode >> bit_pos) & 0b111111;

				if(bit_pos != 0 && inst.inst == inst_type::MOVE)
				{
					// destination ea in move instruction has swapped mode/register bit fields
					// swap it back
					std::uint8_t mode = ea & 0x7;
					std::uint8_t dest_reg = (ea >> 3) & 0x7;
					ea = (mode << 3) | dest_reg;
				}

				if(!ea_mode_matches(ea, modes, size))
					return false;
				break;
			}

			default:
				return false;
			}
		}
	}

	constexpr static bool size_matches(std::uint16_t value, std::uint8_t pos)
	{
		std::uint8_t size = (value >> pos) & 0b11;
		return size != 0b11; // only 0b11 does not match
	}

	static bool ea_mode_matches(std::uint8_t ea, ea_modes modes, std::uint8_t size)
	{
		auto ea_mode = ea_decoder::decode_mode(ea);

		// general rule, address register is not supporeted with byte size
		if(size == 0b00 && ea_mode == addressing_mode::addr_reg)
			return false;

		return mode_is_supported(modes, ea_mode);
	}
};


const auto opcode_map = opcode_builder::build_opcode_map();

m68k::inst_type opcode_decoder::decode(std::uint16_t opcode)
{
	return opcode_map.at(opcode);
}

} // namespace genesis::m68k
