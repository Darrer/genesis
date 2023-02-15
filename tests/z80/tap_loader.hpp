#ifndef __TAP_LOADER_HPP__
#define __TAP_LOADER_HPP__

#include "string_utils.hpp"
#include "z80/cpu.h"

#include <cstdint>
#include <fstream>
#include <iterator>
#include <string_view>
#include <vector>

namespace test::z80
{

enum tap_flag : std::uint8_t
{
	header = 0x00,
	data = 0xFF,
};

class tap_block
{
public:
	tap_flag flag;
	std::vector<std::uint8_t> data;
	std::uint8_t checksum;
};


enum header_type : std::uint8_t
{
	program,
	number_array,
	character_array,
	code_file
};

class tap_header
{
public:
	tap_header(const tap_block& block)
	{
		if (block.flag != tap_flag::header)
		{
			throw std::runtime_error("tap_header error: expected header tap flag");
		}

		if (block.data.size() != 17)
		{
			throw std::runtime_error("tap_header error: unexpected header length");
		}

		type = (header_type)block.data[0];
		std::copy(block.data.cbegin() + 1, block.data.cbegin() + 11, std::back_inserter(filename));

		assign_16(length, block.data[11], block.data[12]);
		assign_16(param1, block.data[13], block.data[14]);
		assign_16(param2, block.data[15], block.data.at(16));
	}

	header_type type;
	std::string filename;
	std::uint16_t length;
	std::uint16_t param1;
	std::uint16_t param2;

private:
	void assign_16(std::uint16_t& dest, std::uint8_t low, std::uint8_t high)
	{
		dest = (high << 8) | low;
	}
};


std::vector<tap_block> read_tap(std::string tap_path)
{
	std::ifstream fs(tap_path, std::ios_base::binary);
	if (!fs.is_open())
	{
		throw std::runtime_error("read_tap error: failed to open file '" + tap_path + "'");
	}

	std::vector<tap_block> res;

	// TAP files in little-endian - don't need to convert

	while (fs)
	{
		std::uint16_t block_size = 0;
		if (!fs.read(reinterpret_cast<char*>(&block_size), sizeof(block_size)))
		{
			// nothing left to read
			break;
		}

		if (block_size <= 2)
		{
			throw std::runtime_error("read_tap error: block size is too small (should be at least 3 bits long)");
		}

		tap_block block;

		if (!fs.read(reinterpret_cast<char*>(&block.flag), sizeof(block.flag)))
		{
			throw std::runtime_error("read_tap error: corrupted file (cannot read flag)");
		}

		// 1 byte for flag; 1 byte for checksum
		const std::uint16_t data_size = block_size - 2;
		for (std::uint16_t i = 0; i < data_size; ++i)
		{
			char c;
			if (!fs.get(c))
			{
				throw std::runtime_error("read_tap error: corrupted file (cannot read data " + su::hex_str(data_size) +
										 ")");
			}

			block.data.push_back(c);
		}

		block.data.shrink_to_fit();

		if (!fs.read(reinterpret_cast<char*>(&block.checksum), sizeof(block.checksum)))
		{
			throw std::runtime_error("read_tap error: corrupted file (cannot read checksum)");
		}

		res.push_back(std::move(block));
	}

	res.shrink_to_fit();
	return res;
}


void load_tap(std::string tap_path, genesis::z80::cpu& cpu)
{
	auto blocks = read_tap(tap_path);

	for (std::size_t i = 0; i < blocks.size(); ++i)
	{
		auto& block = blocks[i];
		if (block.flag == tap_flag::header)
		{
			tap_header header(block);

			if (i + 1 < blocks.size())
			{
				auto& next_block = blocks[i + 1];
				if (next_block.flag == tap_flag::data)
				{
					// load only code file, 've got no idea what to do with the other types...
					if (header.type == header_type::code_file)
					{
						auto& mem = cpu.memory();
						genesis::z80::memory::address base = header.param1;
						for (std::size_t j = 0; j < next_block.data.size(); ++j)
						{
							mem.write(base + j, next_block.data[j]);
						}

						cpu.registers().PC = base;
						cpu.registers().SP = (std::int16_t)0xFF00;

						return;
					}
				}
			}
		}
	}

	throw std::runtime_error("load_tap error: failed to load tap file (not found header or code file data)");
}

} // namespace test::z80

#endif // __TAP_LOADER_HPP__
