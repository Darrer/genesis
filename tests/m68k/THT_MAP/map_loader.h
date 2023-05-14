#ifndef __M68K_MAP_LOADER_H__
#define __M68K_MAP_LOADER_H__

#include <string>
#include <vector>
#include <cstdint>


struct opcode_entry
{
	std::uint16_t opcode;
	bool is_valid;
	std::string description;
};


std::vector<opcode_entry> load_map(std::string path_to_map);


#endif // __M68K_MAP_LOADER_H__
