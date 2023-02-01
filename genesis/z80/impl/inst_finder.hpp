#ifndef __INST_FINDER_HPP__
#define __INST_FINDER_HPP__

#include <cstdint>
#include <array>

#include "instructions.hpp"


namespace genesis::z80
{

class inst_finder
{
private:
	enum map_index
	{
		single,
		dd,
		fd,
		ed,
		cb,
		count,
	};
	constexpr static std::uint16_t no_index = 0xFFFF;

public:
	inst_finder()
	{
		build_maps();
	}

	instruction fast_search(z80::opcode op1, z80::opcode op2)
	{
		std::uint16_t idx = no_index;
		switch (op1)
		{
		case 0xDD:
			idx = get_idx(map_index::dd, op2);
			break;
		case 0xFD:
			idx = get_idx(map_index::fd, op2);
			break;
		case 0xED:
			idx = get_idx(map_index::ed, op2);
			break;
		case 0xCB:
			idx = get_idx(map_index::cb, op2);
			break;
		default:
			idx = get_idx(map_index::single, op1);
			break;
		}

		if(idx == no_index)
		{
			// return make_nop(op1, op2);
			throw std::runtime_error("fast_search error: unsupported opcode: " + su::hex_str(op1) + ", " + su::hex_str(op2));
		}

		// self-check
		auto inst = instructions[idx];
		if(op1 != inst.opcodes[0] || (inst.opcodes[1] != 0x0 && inst.opcodes[1] != op2))
		{
			throw std::runtime_error("internal error: self-check failed, we popped up a wrong instruction!");
		}

		return instructions[idx];
	}

	instruction linear_search(z80::opcode op1, z80::opcode op2)
	{
		
		for(const auto& inst : instructions)
		{
			if(op1 == inst.opcodes[0] && (inst.opcodes[1] == 0x0 || inst.opcodes[1] == op2))
			{
				return inst;
			}
		}

		throw std::runtime_error("linear_search error, unsupported opcode ("
			+ su::hex_str(op1) + ", " + su::hex_str(op2) + ")");
	}

private:
	instruction make_nop(z80::opcode op1, z80::opcode op2)
	{
		instruction inst = { operation_type::nop, {0x0}, addressing_mode::none, addressing_mode::none };
		switch (op1)
		{
		case 0xDD:
		case 0xFD:
		case 0xED:
		case 0xCB:
			inst.opcodes = {op1, op2};
			return inst;
		
		default:
			inst.opcodes = {op1, 0x0};
			return inst;
		}
	}

private:
	void build_maps()
	{
		for(auto& map : maps)
		{
			for(auto& idx : map)
				idx = no_index;
		}

		for(size_t i = 0; i < std::size(instructions); ++i)
		{
			auto& inst = instructions[i];
			switch (inst.opcodes[0])
			{
			case 0xDD:
				store_idx(map_index::dd, i, inst.opcodes[1]);
				break;
			case 0xFD:
				store_idx(map_index::fd, i, inst.opcodes[1]);
				break;
			case 0xED:
				store_idx(map_index::ed, i, inst.opcodes[1]);
				break;
			case 0xCB:
				store_idx(map_index::cb, i, inst.opcodes[1]);
				break;
			default:
				// so far assume it's 1 byte opcode
				if(inst.opcodes[1] != 0x00)
				{
					throw std::runtime_error("build_maps internal error: unknown 2 byte opcode");
				}
				store_idx(map_index::single, i, inst.opcodes[0]);
			}
		}
	}

	void store_idx(map_index map_idx, std::uint16_t inst_idx, z80::opcode op)
	{
		auto& map = maps[map_idx];
		if(map[op] != no_index)
		{
			throw std::runtime_error("store_idx error: failed to save instruction index - the position is already taken (map " +
				std::to_string(map_idx) + ", op2 " + std::to_string(op) + ")");
		}

		map[op] = inst_idx;
	}

	std::uint16_t get_idx(map_index map_idx, z80::opcode op)
	{
		return maps[map_idx][op];
	}

private:
	// isn't it too much for the stack?
	using map = std::array<std::uint16_t, 0x100>;
	std::array<map, map_index::count> maps;
};

}


#endif // __INST_FINDER_HPP__
