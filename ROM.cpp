#include "ROM.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <exception>


namespace genesis
{

class Read
{
public:
	static std::string string(std::ifstream& f, size_t offset, size_t size)
	{
		std::string str;
		str.resize(size);

		f.seekg(offset);
		f.read(str.data(), size);

		// TODO: trim str
		return str;
	}

	// TODO: add big/little endian correction
	template<class T>
	static T builtin_type(std::ifstream& f, size_t offset)
	{
		T data;

		f.seekg(offset);
		f.read(reinterpret_cast<char*>(&data), sizeof(T));

		return data;
	}
};


ROM::ROM(std::string path_to_rom)
{
	std::ifstream f(path_to_rom, std::ios_base::binary);
	if(!f.is_open())
		throw std::runtime_error("Failed to parse ROM: file '" + path_to_rom + "' not found");

	// read data
	_header.sys_type = Read::string(f, 0x100, 16);
	_header._copyright = Read::string(f, 0x110, 16);
	_header.gn_domestic = Read::string(f, 0x120, 48);
	_header.gn_overseas = Read::string(f, 0x150, 48);

	_header._rom_checksum = Read::builtin_type<uint16_t>(f, 0x18E);

	_header._rom_start_addr = Read::builtin_type<uint32_t>(f, 0x1A0);
	_header._rom_end_addr = Read::builtin_type<uint32_t>(f, 0x1A4);

	_header._ram_start_addr = Read::builtin_type<uint32_t>(f, 0x1A8);
	_header._ram_end_addr = Read::builtin_type<uint32_t>(f, 0x1AC);	
}


void print_rom_header(std::ostream& os, const ROMHeader& header)
{
	os << "Sys type: '" << header.system_type() << "'" << std::endl;
	os << "Copyright: '" << header.copyright() << "'" << std::endl;
	os << "Game name (domestic): '" << header.game_name_domestic() << "'" << std::endl;
	os << "Game name (overseas) '" << header.game_name_overseas() << "'" << std::endl;

	// TODO: move to string_utils?
	auto _hex = [](auto t)
	{
		std::stringstream ss;
		ss << "0x";
		ss << std::hex << std::setfill('0') << std::setw(sizeof(t) * 2);
		ss << t;
		return ss.str();
	};

	os << "ROM checksum: "  << _hex(header.rom_checksum()) << std::endl;
	os << "ROM address range: " << _hex(header.rom_start_addr()) << " - " << _hex(header.rom_end_addr()) << std::endl;
	os << "RAM address range: " << _hex(header.ram_start_addr()) << " - " << _hex(header.ram_end_addr()) << std::endl;
}

};
