#include "ROM.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <exception>


namespace genesis
{

using ExtentionList = std::vector<std::string>;
using ParseFunction = ROM(*)(std::ifstream& f);


class ROMParser
{
public:
	virtual ExtentionList supported_extentions() const = 0;
	virtual ROMHeader parse_header(std::ifstream&) const = 0;

protected:
	static std::string read_string(std::ifstream& f, size_t offset, size_t size)
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
	static T read_builtin_type(std::ifstream& f, size_t offset)
	{
		T data;

		f.seekg(offset);
		f.read(reinterpret_cast<char*>(&data), sizeof(T));

		return data;
	}
};


// rename to *Parser
class BinROMParser : public ROMParser
{
public:
	ExtentionList supported_extentions() const override
	{
		return {".bin", ".md"};
	}

	ROMHeader parse_header(std::ifstream& f) const override
	{
		return ROMHeader
		{
			.system_type = read_string(f, 0x100, 16),
			.copyright = read_string(f, 0x110, 16),
			.game_name_domestic = read_string(f, 0x120, 48),
			.game_name_overseas = read_string(f, 0x150, 48),

			.rom_checksum = read_builtin_type<uint16_t>(f, 0x18E),

			.rom_start_addr = read_builtin_type<uint32_t>(f, 0x1A0),
			.rom_end_addr = read_builtin_type<uint32_t>(f, 0x1A4),

			.ram_start_addr = read_builtin_type<uint32_t>(f, 0x1A8),
			.ram_end_addr = read_builtin_type<uint32_t>(f, 0x1AC)
		};
	}

} BinROMParser;


static auto registered_parsers = 
{
	BinROMParser
};


const ROMParser* find_parser(const std::string& extention)
{
	auto is_support_ext = [&](const ROMParser& p)
	{
		auto ext = p.supported_extentions();
		return std::find(ext.begin(), ext.end(), extention) != ext.end();
	};

	auto it = std::find_if(registered_parsers.begin(), registered_parsers.end(), is_support_ext);

	if(it == registered_parsers.end())
		return nullptr;
	return &(*it);
}


ROM::ROM(const std::string_view path_to_rom)
{
	std::filesystem::path rom_path(path_to_rom);
	auto extention = rom_path.extension().string();

	auto parser = find_parser(extention);
	if(parser == nullptr)
	{
		throw std::runtime_error("Faild to parse ROM: extention '" + extention + "' is not supported");
	}

	std::ifstream fs(rom_path, std::ios_base::binary);
	if(!fs.is_open())
	{
		throw std::runtime_error("Failed to open ROM: file '" + rom_path.string() + "' not found");
	}

	_header = parser->parse_header(fs);
}


void print_rom_header(std::ostream& os, const ROMHeader& header)
{
	os << "Sys type: '" << header.system_type << "'" << std::endl;
	os << "Copyright: '" << header.copyright << "'" << std::endl;
	os << "Game name (domestic): '" << header.game_name_domestic << "'" << std::endl;
	os << "Game name (overseas) '" << header.game_name_overseas << "'" << std::endl;

	// TODO: move to string_utils?
	auto _hex = [](auto t)
	{
		std::stringstream ss;
		ss << "0x";
		ss << std::hex << std::setfill('0') << std::setw(sizeof(t) * 2);
		ss << t;
		return ss.str();
	};

	os << "ROM checksum: "  << _hex(header.rom_checksum) << std::endl;
	os << "ROM address range: " << _hex(header.rom_start_addr) << " - " << _hex(header.rom_end_addr) << std::endl;
	os << "RAM address range: " << _hex(header.ram_start_addr) << " - " << _hex(header.ram_end_addr) << std::endl;
}

};
