#include <gtest/gtest.h>

#include "opcode_loader.h"
#include "m68k/impl/opcode_decoder.h"

using namespace genesis::m68k;
using namespace genesis::test;

TEST(M68K, OPCODE_DECODER)
{
	auto tests = load_opcode_tests(R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\decoding\OPCLOGR3.BIN)");
	ASSERT_EQ(tests.size(), 0x10000);

	std::uint16_t failed = 0;

	for(auto test: tests)
	{
		auto inst = opcode_decoder::decode(test.opcode);
		bool is_valid = inst != inst_type::NONE;

		bool expected = test.is_valid;
		bool actual = is_valid;

		if(expected != actual)
			++failed;

		EXPECT_EQ(expected, actual) << "failed to decode " << test.opcode << ", decoded to " << (int)inst;
	}

	ASSERT_EQ(failed, 0);
}
