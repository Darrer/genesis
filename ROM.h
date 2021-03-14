#include <string>

namespace genesis
{

class ROM;

class ROMHeader
{
public:
    ROMHeader() = default;

    std::string system_type() const { return sys_type; }
    std::string copyright() const { return _copyright; }
    std::string game_name_domestic() const { return gn_domestic; }
    std::string game_name_overseas() const { return gn_overseas; }

private:
    std::string sys_type;
    std::string _copyright;
    std::string gn_domestic;
    std::string gn_overseas;

    friend ROM;
};


class ROM
{
public:
    ROM(std::string path_to_rom);

    inline const ROMHeader& header() { return _header; }

private:
    ROMHeader _header;
};


// debug functions
void print_rom_header(std::ostream& os, const ROMHeader& header);

};
