#include "ROM.h"

#include <iostream>


void print_usage(char* prog_path)
{
	std::cout << "Usage ./" << prog_path << " <path to rom>" << std::endl;
}

int main(int args, char* argv[])
{
	if(args != 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	std::string rom_path = argv[1];

	try
	{
		std::cout << "Going to parse: " << rom_path << std::endl;
		genesis::ROM rom(rom_path);
		
		genesis::rom::debug::print_rom_header(std::cout, rom.header());
		genesis::rom::debug::print_rom_vectors(std::cout, rom.vectors());
	}
	catch(std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
