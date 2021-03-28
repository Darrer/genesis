#include <string>
#include <inttypes.h>
#include <filesystem>


namespace genesis
{

// add namespace rom?

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
	// TODO: array of interupt's adressess
	// TODO: array of body data

private:
	ROMHeader _header;
};


namespace debug
{
	void print_rom_header(std::ostream& os, const ROMHeader& header);
}

};
