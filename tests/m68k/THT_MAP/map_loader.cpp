#include "map_loader.h"

#include <fstream>
#include <string_view>
#include <sstream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;


opcode_entry parse_opcode(std::string_view opcode, std::string_view desc)
{
	opcode_entry entry;

	std::stringstream ss;
	ss << std::hex << opcode;
	ss >> entry.opcode;

	entry.is_valid = desc != "None";
	entry.description = desc;

	return entry;
}

std::vector<opcode_entry> load_map(std::string path_to_map)
{
	std::ifstream fs(path_to_map);
	if(!fs.is_open())
		throw std::runtime_error("load_map error: failed to open " + path_to_map);

	json data = json::parse(fs);

	std::vector<opcode_entry> opcodes;

	for(auto& [opcode, desc] : data.items())
	{
		opcodes.push_back(parse_opcode(opcode, desc.get<std::string>()));
	}
		

	return opcodes;
}
