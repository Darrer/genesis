#include "opcode_decoder.h"
#include <array>
#include <algorithm>

namespace genesis::m68k
{

constexpr bool validate_opcodes()
{
	for(auto it = std::begin(opcodes); it != std::end(opcodes); ++it)
	{
		auto entry = *it;

		if(entry.inst_template.size() != 16)
			return false;

		for(auto ch : entry.inst_template)
		{
			if(ch != '1' && ch != '0' && ch != '_')
				return false;
		}

		// check for duplicates
		for(auto next = std::next(it); next != std::end(opcodes); ++next)
		{
			auto next_entry = *next;
			if(entry.inst_template == next_entry.inst_template)
				return false;
			
			// so far also assume there should be no duplicate instructions
			if(entry.inst == next_entry.inst)
				return false;
		}
	}

	return true;
}

static_assert(validate_opcodes());

struct opcode_entry
{
	std::uint16_t mask;
	std::uint16_t opcode;
	inst_type inst;
};

class opcode_builder
{
public:
	opcode_builder() = delete;

	constexpr static std::array<opcode_entry, std::size(opcodes)> build_opcode_map()
	{
		std::array<opcode_entry, std::size(opcodes)> res;

		std::size_t pos = 0;
		for(auto entry : opcodes)
		{
			auto mask = gen_mask(entry.inst_template);
			auto opcode = gen_opcode(entry.inst_template);

			res.at(pos++) = { mask, opcode, entry.inst };
		}

		// instructions with longest mask should come first
		std::sort(res.begin(), res.end(), [](opcode_entry a, opcode_entry b)
		{
			return a.mask > b.mask;
		});

		return res;
	}

	constexpr static std::uint16_t gen_mask(std::string_view inst_template)
	{
		std::uint16_t mask = 0;
		std::uint8_t pos = 0;
		for(auto it = inst_template.rbegin(); it != inst_template.rend(); ++it)
		{
			auto ch = *it;
			if(ch == '1' || ch == '0')
			{
				mask |= 1 << pos;
			}
			++pos;
		}
		return mask;
	}

	constexpr static std::uint16_t gen_opcode(std::string_view inst_template)
	{
		std::uint16_t mask = 0;
		std::uint8_t pos = 0;
		for(auto it = inst_template.rbegin(); it != inst_template.rend(); ++it)
		{
			auto ch = *it;
			if(ch == '1')
			{
				mask |= 1 << pos;
			}
			++pos;
		}
		return mask;
	}
};


constexpr auto inst_map = opcode_builder::build_opcode_map();

m68k::inst_type opcode_decoder::decode(std::uint16_t opcode)
{
	for(auto entry : inst_map)
	{
		// std::cout << "Checking for " << entry.mask << " " << entry.opcode << " " << (int)entry.inst << std::endl;
		if((opcode & entry.mask) == entry.opcode)
			return entry.inst;
	}

	return inst_type::NONE;
}

}
