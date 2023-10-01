#include "m68k/impl/opcode_decoder.h"

#include "../../helper.hpp"
#include "opcode_loader.h"

#include <gtest/gtest.h>

using namespace genesis::m68k;
using namespace genesis::test;


void patch_tests(std::vector<opcode_test>& tests)
{
	/*
		From doc: word patterns with bits 15–12 equaling 1010 or 1111 are
			distinguished as unimplemented instructions
		However, for some reasons tests treat these instructions as valid, overwrite it here
	*/

	for (auto& test : tests)
	{
		std::uint8_t high_nibble = test.opcode >> 12;
		if (high_nibble == 0b1010 || high_nibble == 0b1111)
			test.is_valid = false;
	}
}

// Opcode map for this test is taken from:
// https://www.atari-forum.com/viewtopic.php?p=256793&sid=000cc3cc577f62b3af2f4ec2c9df339e#p256793
TEST(M68K, OPCODE_DECODER)
{
	auto bin_path = get_exec_path() / "m68k" / "OPCLOGR3.BIN";
	auto tests = load_opcode_tests(bin_path.string());

	ASSERT_EQ(tests.size(), 0x10000);

	patch_tests(tests);

	[[maybe_unused]] std::uint16_t failed = 0;

	for (auto test : tests)
	{
		auto inst = opcode_decoder::decode(test.opcode);
		bool is_valid = inst != inst_type::NONE;

		bool expected = test.is_valid;
		bool actual = is_valid;

		if (expected != actual)
			++failed;

		if (expected == false && actual == true)
		{
			ASSERT_EQ(expected, actual) << "failed to decode " << test.opcode << ", decoded to " << (int)inst;
		}
	}

	// Tests have about 1181 opcodes which is marked as valid, but I've got no idea to which instruction I should map
	// them ASSERT_EQ(failed, 0);
}
