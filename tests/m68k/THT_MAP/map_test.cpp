#include <gtest/gtest.h>

#include "m68k/impl/opcode_decoder.h"
#include "map_loader.h"
#include "../../helper.hpp"

using namespace genesis::m68k;

// The opcode list is taken from
// https://github.com/TomHarte/ProcessorTests
TEST(M68K, THT_MAP)
{
	auto map_path = get_exec_path() / "m68k" / "68000.official.json";
	auto opcodes = load_map(map_path.string());

	ASSERT_EQ(opcodes.size(), 0x10000);

	for(auto& entry : opcodes)
	{
		auto inst = opcode_decoder::decode(entry.opcode);

		bool expected = entry.is_valid;
		bool actuall = inst != inst_type::NONE;

		ASSERT_EQ(expected, actuall) << entry.description;
	}
}
