#include <gtest/gtest.h>

#include "opcode_loader.h"
#include "m68k/impl/opcode_decoder.h"

using namespace genesis::m68k;
using namespace genesis::test;


void patch_tests(std::vector<opcode_test>& tests)
{
	/* 
		From doc: word patterns with bits 15–12 equaling 1010 or 1111 are
			distinguished as unimplemented instructions
		However, for some reasons tests treat these instructions as valid, overwrite it here
	*/

	for(auto& test : tests)
	{
		std::uint8_t high_nibble = test.opcode >> 12;
		if(high_nibble == 0b1010 || high_nibble == 0b1111)
			test.is_valid = false;

		if(test.opcode == 0b0100101011111100)
		{
			// it's ILLEGAL instruction
			test.is_valid = true;
		}
	}
}

TEST(M68K, OPCODE_DECODER)
{
	auto tests = load_opcode_tests(R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\decoding\OPCLOGR3.BIN)");
	ASSERT_EQ(tests.size(), 0x10000);

	patch_tests(tests);

	std::uint16_t failed = 0;

	for(auto test: tests)
	{
		auto inst = opcode_decoder::decode(test.opcode);
		bool is_valid = inst != inst_type::NONE;

		bool expected = test.is_valid;
		bool actual = is_valid;

		if(expected != actual)
			++failed;

		if(expected == false && actual == true)
			EXPECT_EQ(expected, actual) << "failed to decode " << test.opcode << ", decoded to " << (int)inst;
	}

	ASSERT_EQ(failed, 0);
}
