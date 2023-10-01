#ifndef __ROM_DEBUG_HPP_
#define __ROM_DEBUG_HPP_

#include "rom.h"
#include "string_utils.hpp"

#include <ostream>


namespace genesis::debug
{

[[maybe_unused]] static void print_rom_header(std::ostream& os, const rom::header_data& header)
{
	os << "Sys type: '" << header.system_type << "'" << std::endl;
	os << "Copyright: '" << header.copyright << "'" << std::endl;
	os << "Game name (domestic): '" << header.game_name_domestic << "'" << std::endl;
	os << "Game name (overseas): '" << header.game_name_overseas << "'" << std::endl;

	using namespace su;
	os << "ROM checksum: " << hex_str(header.rom_checksum) << std::endl;
	os << "ROM address range: " << hex_str(header.rom_start_addr) << " - " << hex_str(header.rom_end_addr) << std::endl;
	os << "RAM address range: " << hex_str(header.ram_start_addr) << " - " << hex_str(header.ram_end_addr) << std::endl;
}


template <class array, class format_func>
static void print_array(std::ostream& os, const array& arr, format_func fn, size_t elments_per_row)
{
	size_t printed = 0;
	for(const auto& el : arr)
	{
		os << fn(el);
		++printed;

		if((printed % elments_per_row == 0) || printed == arr.size())
			os << '\n';
		else
			os << ' ';
	}
}


template <class array>
static void print_hex_array(std::ostream& os, const array& arr, size_t elements_per_row)
{
	auto format_fn = [](const auto& el) { return su::hex_str(el); };
	print_array(os, arr, format_fn, elements_per_row);
}


[[maybe_unused]] static void print_rom_vectors(std::ostream& os, const rom::vector_array& vectors)
{
	print_hex_array(os, vectors, 4);
}


[[maybe_unused]] static void print_rom_body(std::ostream& os, const rom::byte_array& body)
{
	auto format_fn = [](const auto& addr) { return su::hex_str<int>(static_cast<unsigned>(addr), sizeof(addr) * 2); };
	print_array(os, body, format_fn, 16);
}

} // namespace genesis::debug

#endif // __ROM_DEBUG_HPP_
