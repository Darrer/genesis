#include "ROM.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <exception>
#include <cassert>


namespace genesis
{

using ExtentionList = std::vector<std::string>;

class ROMParser
{
public:
	virtual ExtentionList supported_extentions() const = 0;

	virtual ROMHeader parse_header(std::ifstream&) const = 0;
	virtual VectorList parse_vectors(std::ifstream&) const = 0;
	virtual Body parse_body(std::ifstream&) const = 0;

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

	VectorList parse_vectors(std::ifstream& f) const override
	{
		VectorList vectors;

		size_t vec_num = 0;

		std::generate(vectors.begin(), vectors.end(), [&]() {
			return read_builtin_type<uint32_t>(f, vec_num++ * sizeof(uint32_t));
		});


		assert(vec_num * sizeof(uint32_t) == 0x100);
		assert(vectors.size() == 64);

		return vectors;
	}

	Body parse_body(std::ifstream& f) const override
	{
		Body body;

		f.seekg(0x200);
		while(f)
		{
			char c;
			f.get(c);

			body.push_back(static_cast<uint8_t>(c));
		}

		body.shrink_to_fit();
		return body;
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
	_vectors = parser->parse_vectors(fs);
	_body = parser->parse_body(fs);
}


uint16_t ROM::checksum() const
{
	auto calc_chksum = [this]()
	{
		size_t num_to_check = _body.size();
		if(num_to_check > 0 && num_to_check % 2 != 0)
			--num_to_check;

		uint16_t chksum = 0;
		for(size_t i = 0; i < num_to_check; ++i)
		{
			if(i % 2 == 0)
				chksum += _body[i] * (uint16_t)256;
			else
				chksum += _body[i];
		}

		return chksum;
	};

	if(saved_checksum == 0)
		saved_checksum = calc_chksum();

	return saved_checksum;
}


namespace rom::debug
{

#include "string_utils.hpp"

void print_rom_header(std::ostream& os, const ROMHeader& header)
{
	os << "Sys type: '" << header.system_type << "'" << std::endl;
	os << "Copyright: '" << header.copyright << "'" << std::endl;
	os << "Game name (domestic): '" << header.game_name_domestic << "'" << std::endl;
	os << "Game name (overseas) '" << header.game_name_overseas << "'" << std::endl;

	os << "ROM checksum: "  << hex_str(header.rom_checksum) << std::endl;
	os << "ROM address range: " << hex_str(header.rom_start_addr) << " - " << hex_str(header.rom_end_addr) << std::endl;
	os << "RAM address range: " << hex_str(header.ram_start_addr) << " - " << hex_str(header.ram_end_addr) << std::endl;
}

void print_rom_vectors(std::ostream& os, const VectorList& vectors)
{
	const size_t addr_per_row = 4;

	size_t printed = 0;
	auto dump_addr = [&](auto vec)
	{
		os << hex_str(vec);
		++printed;

		if( (printed % addr_per_row == 0) || printed == vectors.size() )
			os << '\n';
		else
			os << ' ';
	};

	std::for_each(vectors.cbegin(), vectors.cend(), dump_addr);
}

void print_rom_body(std::ostream&os, const Body& body)
{
	const size_t bytes_per_row = 16;

	size_t printed = 0;
	auto dump_addr = [&](uint8_t addr)
	{
		os << hex_str<int>(static_cast<unsigned int>(addr), sizeof(addr)*2);
		++printed;

		if( (printed % bytes_per_row == 0) || printed == body.size() )
			os << '\n';
		else
			os << ' ';
	};

	std::for_each(body.cbegin(), body.cend(), dump_addr);
}

}

};
