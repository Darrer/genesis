#ifndef __M68K_OPCODE_LOADER_H__
#define __M68K_OPCODE_LOADER_H__

#include <cstdint>
#include <vector>
#include <string>

namespace genesis::test
{

struct opcode_test
{
	std::uint16_t opcode;
	bool is_valid;
};


std::vector<opcode_test> load_opcode_tests(std::string path);

}

#endif // __M68K_OPCODE_LOADER_H__
