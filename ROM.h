#include <string>
#include <inttypes.h>


namespace genesis
{

// TODO: add ROM parse_rom(path) function

class ROM;

class ROMHeader
{
public:
	ROMHeader() = default;

	std::string system_type() const { return sys_type; }
	std::string copyright() const { return _copyright; }
	std::string game_name_domestic() const { return gn_domestic; }
	std::string game_name_overseas() const { return gn_overseas; }

	uint16_t rom_checksum() const { return _rom_checksum; }

	uint32_t rom_start_addr() const { return _rom_start_addr; }
	uint32_t rom_end_addr() const { return _rom_end_addr; }

	uint32_t ram_start_addr() const { return _ram_start_addr; }
	uint32_t ram_end_addr() const { return _ram_end_addr; }

private:
	std::string sys_type;
	std::string _copyright;
	std::string gn_domestic;
	std::string gn_overseas;

	uint16_t _rom_checksum;

	uint32_t _rom_start_addr;
	uint32_t _rom_end_addr;

	uint32_t _ram_start_addr;
	uint32_t _ram_end_addr;

	friend ROM;
};


class ROM
{
	// TODO: move ROMHeader here
public:
	ROM(std::string path_to_rom);

	inline const ROMHeader& header() { return _header; }

private:
	ROMHeader _header;
};

// TODO: add class BinROMReader - which will read ROMs in .bin format


// debug functions
void print_rom_header(std::ostream& os, const ROMHeader& header);

};
