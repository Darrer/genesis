#ifndef __ROM_DEBUG_HPP_
#define __ROM_DEBUG_HPP_

#include "ROM.h"
#include "string_utils.hpp"


namespace genesis::rom::debug
{

static void print_rom_header(std::ostream& os, const ROMHeader& header)
{
	os << "Sys type: '" << header.system_type << "'" << std::endl;
	os << "Copyright: '" << header.copyright << "'" << std::endl;
	os << "Game name (domestic): '" << header.game_name_domestic << "'" << std::endl;
	os << "Game name (overseas) '" << header.game_name_overseas << "'" << std::endl;

	os << "ROM checksum: " << hex_str(header.rom_checksum) << std::endl;
	os << "ROM address range: " << hex_str(header.rom_start_addr) << " - " << hex_str(header.rom_end_addr) << std::endl;
	os << "RAM address range: " << hex_str(header.ram_start_addr) << " - " << hex_str(header.ram_end_addr) << std::endl;
}

static void print_rom_vectors(std::ostream& os, const VectorList& vectors)
{
	const size_t addr_per_row = 4;

	size_t printed = 0;
	auto dump_addr = [&](auto vec) {
		os << hex_str(vec);
		++printed;

		if ((printed % addr_per_row == 0) || printed == vectors.size())
			os << '\n';
		else
			os << ' ';
	};

	std::for_each(vectors.cbegin(), vectors.cend(), dump_addr);
}

static void print_rom_body(std::ostream& os, const Body& body)
{
	const size_t bytes_per_row = 16;

	size_t printed = 0;
	auto dump_addr = [&](uint8_t addr) {
		os << hex_str<int>(static_cast<unsigned int>(addr), sizeof(addr) * 2);
		++printed;

		if ((printed % bytes_per_row == 0) || printed == body.size())
			os << '\n';
		else
			os << ' ';
	};

	std::for_each(body.cbegin(), body.cend(), dump_addr);
}

} // namespace genesis::rom::debug

#endif // __ROM_DEBUG_HPP_
