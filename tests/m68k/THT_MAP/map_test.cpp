#include <gtest/gtest.h>

#include "m68k/impl/opcode_decoder.h"
#include "map_loader.h"

using namespace genesis::m68k;


TEST(M68K, THT_MAP)
{
	std::string map_path = R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\68000.official.json)";
	auto opcodes = load_map(map_path);

	ASSERT_EQ(opcodes.size(), 0x10000);

	for(auto& entry : opcodes)
	{
		auto inst = opcode_decoder::decode(entry.opcode);

		bool expected = entry.is_valid;
		bool actuall = inst != inst_type::NONE;

		ASSERT_EQ(expected, actuall) << entry.description;
	}
}
