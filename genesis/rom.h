#ifndef __ROM_H__
#define __ROM_H__

#include <array>
#include <cstdint>
#include <vector>
#include <span>
#include <optional>
#include <string_view>


namespace genesis
{

class rom
{
public:
	static const constexpr std::size_t MAX_SIZE = 0x400000;
	static const constexpr std::size_t MIN_SIZE = 0x201; // header + vector + 1 body byte

	struct header_data
	{
		bool operator==(const header_data&) const = default;

		std::string_view system_type;
		std::string_view copyright;
		std::string_view game_name_domestic;
		std::string_view game_name_overseas;
		std::string_view region_support;

		std::uint16_t rom_checksum;

		std::uint32_t rom_start_addr;
		std::uint32_t rom_end_addr;

		std::uint32_t ram_start_addr;
		std::uint32_t ram_end_addr;
	};

	using vector_array = std::array<std::uint32_t, 64>;

public:
	rom(std::string_view path_to_rom);

	std::span<const std::uint8_t> data() const
	{
		return {m_rom_data.begin(), m_rom_data.size()};
	}

	// NOTE: rom class is supposed to be immutable, but keep this method for now
	std::span<std::uint8_t> data()
	{
		return {m_rom_data.begin(), m_rom_data.size()};
	}

	const header_data& header() const
	{
		return m_header;
	}

	const vector_array& vectors() const
	{
		return m_vectors;
	}

	std::span<const std::uint8_t> body() const
	{
		const std::size_t BODY_OFFSET = MIN_SIZE - 1;

		if(m_rom_data.size() <= BODY_OFFSET)
			return {}; // no body

		return {m_rom_data.begin() + BODY_OFFSET, m_rom_data.size() - BODY_OFFSET};
	}

	std::uint16_t checksum() const;

private:
	void setup_header();
	void setup_vectors();

private:
	std::vector<std::uint8_t> m_rom_data;
	mutable std::optional<std::uint16_t> m_checksum;

	header_data m_header;
	vector_array m_vectors;
};

} // namespace genesis

#endif // __ROM_H__
