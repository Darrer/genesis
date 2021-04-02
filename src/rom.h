#ifndef __ROM_H__
#define __ROM_H__

#include <array>
#include <filesystem>
#include <inttypes.h>
#include <string>

namespace genesis
{

class rom
{
public:
	struct header_data
	{
	public:
		std::string system_type;
		std::string copyright;
		std::string game_name_domestic;
		std::string game_name_overseas;

		uint16_t rom_checksum;

		uint32_t rom_start_addr;
		uint32_t rom_end_addr;

		uint32_t ram_start_addr;
		uint32_t ram_end_addr;
	};

	using vector_array = std::array<uint32_t, 64>;
	using byte_array = std::vector<uint8_t>;

public:
	rom(const std::string_view path_to_rom);

	inline const header_data& header() const
	{
		return _header;
	}

	inline const vector_array& vectors() const
	{
		return _vectors;
	}

	inline const byte_array& body() const
	{
		return _body;
	}

	uint16_t checksum() const;

private:
	header_data _header;
	vector_array _vectors;
	byte_array _body;

	mutable uint16_t saved_checksum = 0;
};

} // namespace genesis

#endif // __ROM_H__
