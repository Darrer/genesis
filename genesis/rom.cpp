#include "rom.h"

#include "endian.hpp"
#include "string_utils.hpp"

#include <cassert>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>


namespace genesis
{

// add namespace
const std::string rom_format_err_msg = "ROM format error!";

class rom_parser
{
public:
	using extention_list = std::vector<std::string_view>;

public:
	virtual ~rom_parser() = default;

	virtual extention_list supported_extentions() const = 0;

	virtual rom::header_data parse_header(std::ifstream&) const = 0;
	virtual rom::vector_array parse_vectors(std::ifstream&) const = 0;
	virtual rom::byte_array parse_body(std::ifstream&) const = 0;

protected:
	static std::string read_string(std::ifstream& f, size_t offset, size_t size)
	{
		std::string str;
		str.resize(size);

		f.seekg(offset);
		f.read(str.data(), size);

		check_stream(f);

		su::trim(str);
		return str;
	}

	template <class T>
	static T read_builtin_type(std::ifstream& f, size_t offset)
	{
		T data{};

		f.seekg(offset);
		f.read(reinterpret_cast<char*>(&data), sizeof(T));

		check_stream(f);

		// NOTE: rom has big-endian format
		endian::big_to_sys(data);
		return data;
	}

	static inline void check_stream(std::ifstream& f)
	{
		if (!f.good())
			throw std::runtime_error(rom_format_err_msg);
	}
};


class bin_rom_parser : public rom_parser
{
public:
	extention_list supported_extentions() const override
	{
		return {".bin", ".md"};
	}

	rom::header_data parse_header(std::ifstream& f) const override
	{
		return rom::header_data{.system_type = read_string(f, 0x100, 16),
								.copyright = read_string(f, 0x110, 16),
								.game_name_domestic = read_string(f, 0x120, 48),
								.game_name_overseas = read_string(f, 0x150, 48),

								.rom_checksum = read_builtin_type<uint16_t>(f, 0x18E),

								.rom_start_addr = read_builtin_type<uint32_t>(f, 0x1A0),
								.rom_end_addr = read_builtin_type<uint32_t>(f, 0x1A4),

								.ram_start_addr = read_builtin_type<uint32_t>(f, 0x1A8),
								.ram_end_addr = read_builtin_type<uint32_t>(f, 0x1AC)};
	}

	rom::vector_array parse_vectors(std::ifstream& f) const override
	{
		rom::vector_array vectors;

		size_t vec_num = 0;
		std::generate(vectors.begin(), vectors.end(),
					  [&]() { return read_builtin_type<uint32_t>(f, vec_num++ * sizeof(uint32_t)); });

		assert(vec_num * sizeof(uint32_t) == 0x100);
		assert(vectors.size() == 64);

		return vectors;
	}

	rom::byte_array parse_body(std::ifstream& f) const override
	{
		rom::byte_array body;

		f.seekg(0x200);
		while (f)
		{
			char c;
			if (f.get(c))
				body.push_back(static_cast<uint8_t>(c));
		}

		// body cannot be empty
		if (body.empty())
			throw std::runtime_error(rom_format_err_msg);

		body.shrink_to_fit();
		return body;
	}

};


// static rom_parser* registered_parsers = { new bin_rom_parser()};
std::array<std::shared_ptr<rom_parser>, 1> registered_parsers = 
{
	std::make_shared<bin_rom_parser>()
};


const std::shared_ptr<rom_parser> find_parser(const std::string& extention)
{
	auto is_support_ext = [&](auto p) {
		auto ext = p->supported_extentions();
		return std::find(ext.begin(), ext.end(), extention) != ext.end();
	};

	auto it = std::find_if(registered_parsers.begin(), registered_parsers.end(), is_support_ext);

	if (it == registered_parsers.end())
		return nullptr;
	return *it;
}


rom::rom(const std::string_view path_to_rom)
{
	std::filesystem::path rom_path(path_to_rom);
	auto extention = rom_path.extension().string();

	auto parser = find_parser(extention);
	if (parser == nullptr)
	{
		throw std::runtime_error("faild to parse ROM: extention '" + extention + "' is not supported");
	}

	std::ifstream fs(rom_path, std::ios_base::binary);
	if (!fs.is_open())
	{
		throw std::runtime_error("failed to open ROM: file '" + rom_path.string() + "'");
	}

	_header = parser->parse_header(fs);
	_vectors = parser->parse_vectors(fs);
	_body = parser->parse_body(fs);
}


uint16_t rom::checksum() const
{
	auto calc_chksum = [this]() {
		size_t num_to_check = _body.size();
		if (num_to_check > 0 && num_to_check % 2 != 0)
			--num_to_check;

		uint16_t chksum = 0;
		for (size_t i = 0; i < num_to_check; ++i)
		{
			if (i % 2 == 0)
				chksum += _body[i] * (uint16_t)256;
			else
				chksum += _body[i];
		}

		return chksum;
	};

	if (saved_checksum == 0)
		saved_checksum = calc_chksum();

	return saved_checksum;
}

} // namespace genesis
