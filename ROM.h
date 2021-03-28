#include <string>
#include <inttypes.h>
#include <filesystem>
#include <array>


namespace genesis
{

// add namespace rom?

using VectorList = std::array<uint32_t, 64>;

struct ROMHeader
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


class ROM
{
	// TODO: move ROMHeader here
public:
	ROM(const std::string_view path_to_rom);

	inline const ROMHeader& header() const { return _header; }
	inline const VectorList& vectors() const { return _vectors; }

	// TODO: array of body data
	// TODO: calc actual checksum?

private:
	ROMHeader _header;
	VectorList _vectors;
};


// TODO: move to separate file
namespace rom::debug
{
	void print_rom_header(std::ostream& os, const ROMHeader& header);
	void print_rom_vectors(std::ostream& os, const VectorList& vectors);
}

};
