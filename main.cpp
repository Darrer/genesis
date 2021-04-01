#include <iostream>

#include "ROM.h"
#include "ROM_debug.hpp"
#include "string_utils.hpp"


void print_usage(char* prog_path)
{
	std::cout << "Usage ./" << prog_path << " <path to rom>" << std::endl;
}

int main(int args, char* argv[])
{
	if (args != 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	std::string rom_path = argv[1];

	try
	{
		std::cout << "Going to parse: " << rom_path << std::endl;
		genesis::ROM rom(rom_path);

		std::ostream& os = std::cout;
		genesis::rom::debug::print_rom_header(os, rom.header());
		os << "Actual checksum: " << hex_str(rom.checksum()) << std::endl;

		genesis::rom::debug::print_rom_vectors(os, rom.vectors());
		genesis::rom::debug::print_rom_body(os, rom.body());
	}
	catch (std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
