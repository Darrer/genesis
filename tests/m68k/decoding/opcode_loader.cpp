#include "opcode_loader.h"
#include "endian.hpp"

#include <fstream>
#include <limits>
#include <iostream>


namespace genesis::test
{

std::vector<opcode_test> load_opcode_tests(std::string path)
{
	std::ifstream fs(path, std::ios_base::binary);
	if(!fs.is_open())
		throw std::runtime_error("load_opcode_tests error: failed to open " + path);

	std::vector<opcode_test> res;
	std::uint16_t expected_opcode = 0;
	while (fs)
	{
		// Every entry is 4 bytes long:
		// 2 bytes is opcode
		// 2 bytes is string (VA or IN)
		opcode_test entry;
		fs.read(reinterpret_cast<char*>(&entry.opcode), sizeof(entry.opcode));

		if(!fs.good())
			throw std::runtime_error("load_opcode_tests error: failed to read opcode");

		endian::big_to_sys(entry.opcode);

		if(entry.opcode != expected_opcode)
			throw std::runtime_error("load_opcode_tests error: read unexpected opcode");

		char str[2];
		fs.read(str, 2);
		if(!fs.good())
			throw std::runtime_error("load_opcode_tests error: failed to read str");

		if(str[0] == 'V' && str[1] == 'A')
			entry.is_valid = true;
		else if(str[0] == 'I' && str[1] == 'N')
			entry.is_valid = false;
		else
			throw std::runtime_error("load_opcode_tests error: unknown str while parsing " + std::to_string(expected_opcode));

		res.push_back(entry);

		if(expected_opcode == 0xFFFF)
			break;
		++expected_opcode;
	}

	return res;
}

}
