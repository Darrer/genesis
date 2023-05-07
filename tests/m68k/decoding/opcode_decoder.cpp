#include <gtest/gtest.h>

#include "opcode_loader.h"
#include "m68k/impl/opcode_decoder.h"

using namespace genesis::m68k;
using namespace genesis::test;

TEST(M68K, OPCODE_DECODER)
{
	auto tests = load_opcode_tests(R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\decoding\OPCLOGR3.BIN)");
	ASSERT_EQ(tests.size(), 0x10000);

	for(auto test: tests)
	{
		auto inst = opcode_decoder::decode(test.opcode);
		bool is_valid = inst != inst_type::NONE;

		ASSERT_EQ(test.is_valid, is_valid) << "failed to decode " << test.opcode << ", decoded to " << (int)inst;
	}
}
