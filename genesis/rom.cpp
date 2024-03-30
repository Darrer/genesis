#include "rom.h"

#include "endian.hpp"
#include "string_utils.hpp"
#include "exception.hpp"

#include <exception>
#include <fstream>
#include <filesystem>
#include <ranges>


namespace genesis
{

class rom_parser
{
public:
	virtual ~rom_parser() = default;

	virtual std::vector<std::string_view> supported_extentions() const = 0;
	virtual std::vector<std::uint8_t> read_raw_rom(std::ifstream&) const = 0;
};

class bin_rom_parser : public rom_parser
{
public:
	std::vector<std::string_view> supported_extentions() const override
	{
		return {".bin", ".md"};
	}

	std::vector<std::uint8_t> read_raw_rom(std::ifstream& fs) const override
	{
		std::vector<std::uint8_t> rom;
		// rom.reserve(rom::MAX_SIZE);

		fs.clear();
		fs.seekg(0);
		for(char c; fs.get(c);)
		{
			rom.push_back(c);
			if(rom.size() > rom::MAX_SIZE)
				throw std::runtime_error("ROM is too big");
		}

		const std::size_t MIN_ROM_SIZE = 0x200 + 1; // there should be at least 1 byte for body
		if(rom.size() < MIN_ROM_SIZE)
			throw std::runtime_error("ROM is too small");

		// TODO: do we need to pad ROM here?
		// pad rom
		// for(std::size_t i = rom.size(); i < rom::MAX_SIZE; ++i)
		// 	rom.push_back(0);

		rom.shrink_to_fit();
		return rom;
	}
};

const rom_parser* find_parser(std::string_view extention)
{
	static std::array<std::unique_ptr<rom_parser>, 1> registered_parsers
	{
		std::make_unique<bin_rom_parser>()
	};

	auto is_support_ext = [&](const auto& p)
	{
		auto exts = p->supported_extentions();
		return std::ranges::contains(exts, extention);
	};

	auto it = std::ranges::find_if(registered_parsers, is_support_ext);
	if(it == registered_parsers.end())
		return nullptr;
	return it->get();
}

rom::rom(std::string_view path_to_rom)
{
	std::filesystem::path rom_path(path_to_rom);
	auto extention = rom_path.extension().string();

	auto parser = find_parser(extention);
	if(parser == nullptr)
	{
		throw std::runtime_error("faild to parse ROM: extention '" + extention + "' is not supported");
	}

	std::ifstream fs(rom_path, std::ios_base::binary);
	if(!fs.is_open())
	{
		throw std::runtime_error("failed to open ROM file '" + rom_path.string() + "'");
	}

	m_rom_data = parser->read_raw_rom(fs);

	setup_header();
	setup_vectors();
}

std::uint16_t rom::checksum() const
{
	static auto calc_chksum = [this]() {
		auto _body = body();
		std::size_t num_to_check = _body.size();
		if(num_to_check > 0 && num_to_check % 2 != 0)
			--num_to_check;

		std::uint16_t chksum = 0;
		for(std::size_t i = 0; i < num_to_check; ++i)
		{
			if(i % 2 == 0)
				chksum += _body[i] * (std::uint16_t)256;
			else
				chksum += _body[i];
		}

		return chksum;
	};

	if(!m_checksum.has_value())
		m_checksum = calc_chksum();

	return m_checksum.value();
}

template <class T>
T read_builtin_type(std::span<const std::uint8_t> buffer, std::size_t offset)
{
	if(offset + sizeof(T) >= buffer.size())
		throw internal_error();
	T data {*reinterpret_cast<const T*>(buffer.data() + offset)};
	endian::big_to_sys(data); // rom has big-endian format
	return data;
}

std::string_view read_string(std::span<const std::uint8_t> buffer, std::size_t offset, std::size_t size)
{
	if(offset + size >= buffer.size())
		throw internal_error();
	std::string_view str {reinterpret_cast<const char*>(buffer.data() + offset), size};
	su::trim(str);
	return str;
}

void rom::setup_header()
{
	m_header.system_type = read_string(m_rom_data, 0x100, 16);
	m_header.copyright = read_string(m_rom_data, 0x110, 16);
	m_header.game_name_domestic = read_string(m_rom_data, 0x120, 48);
	m_header.game_name_overseas = read_string(m_rom_data, 0x150, 48);
	m_header.region_support = read_string(m_rom_data, 0x1F0, 3);

	m_header.rom_checksum = read_builtin_type<std::uint16_t>(m_rom_data, 0x18E);
	m_header.rom_start_addr = read_builtin_type<std::uint32_t>(m_rom_data, 0x1A0);
	m_header.rom_end_addr = read_builtin_type<std::uint32_t>(m_rom_data, 0x1A4);
	m_header.ram_start_addr = read_builtin_type<std::uint32_t>(m_rom_data, 0x1A8);
	m_header.ram_end_addr = read_builtin_type<std::uint32_t>(m_rom_data, 0x1AC);
}

void rom::setup_vectors()
{
	int vec_num = 0;
	std::generate(m_vectors.begin(), m_vectors.end(), [&]() {
		int offset = vec_num++ * sizeof(std::uint32_t);
		return read_builtin_type<std::uint32_t>(m_rom_data, offset);
	});
}

} // namespace genesis
