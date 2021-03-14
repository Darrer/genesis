#include "ROM.h"

#include <iostream>
#include <fstream>


namespace genesis
{

std::string read_string(std::ifstream& f, size_t offset, size_t size)
{
	std::string variable;
	variable.resize(size);

	f.seekg(offset);
	f.read(variable.data(), size);

	return variable;
}

ROM::ROM(std::string path_to_rom)
{
	std::cout << "Going to parse: " << path_to_rom << std::endl;


	std::ifstream f(path_to_rom, std::ios_base::binary);
	if(!f.is_open())
		return;

	// read data
	_header.sys_type = read_string(f, 0x100, 16);
	_header._copyright = read_string(f, 0x110, 16);
	_header.gn_domestic = read_string(f, 0x120, 48);
	_header.gn_overseas = read_string(f, 0x150, 48);
}


void print_rom_header(std::ostream& os, const ROMHeader& header)
{
	os << "Sys type: '" << header.system_type() << "'" << std::endl;
    os << "Copyright: '" << header.copyright() << "'" << std::endl;
    os << "Game name (domestic): '" << header.game_name_domestic() << "'" << std::endl;
    os << "Game name (overseas) '" << header.game_name_overseas() << "'" << std::endl;
}

};
