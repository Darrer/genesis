#include <gtest/gtest.h>
#include <iostream>

#include "tests_loader.h"
#include "m68k/cpu.h"

using namespace genesis;


void set_preconditions(m68k::cpu& cpu, const cpu_state& state)
{
	// prepare registers
	auto& regs = cpu.registers();

	regs.D0.LW = state.D0;
	regs.D1.LW = state.D1;
	regs.D2.LW = state.D2;
	regs.D3.LW = state.D3;
	regs.D4.LW = state.D4;
	regs.D5.LW = state.D5;
	regs.D6.LW = state.D6;
	regs.D7.LW = state.D7;

	regs.A0.LW = state.A0;
	regs.A1.LW = state.A1;
	regs.A2.LW = state.A2;
	regs.A3.LW = state.A3;
	regs.A4.LW = state.A4;
	regs.A5.LW = state.A5;
	regs.A6.LW = state.A6;

	regs.USP.LW = state.USP;
	// regs.SR = state.SR; // skip for now
	regs.PC = state.PC;

	// prepare ram
	auto& mem = cpu.memory();
	for(const auto& ram : state.ram)
		mem.write(ram.addres, ram.value);
	
	// TMP: we don't have prefetch queue yet, so write it to memory pointed by PC
	std::uint32_t offset = 0x0;
	for(auto val : state.prefetch)
	{
		mem.write(regs.PC + offset, val);
		offset += sizeof(val);
	}
}

bool check_postconditions(m68k::cpu& cpu, const cpu_state& state)
{
	bool failed = false;

	#define check_eq(expected, actual) \
		failed = failed || expected != actual; \
		EXPECT_EQ(expected, actual)

	// check registers
	auto& regs = cpu.registers();

	check_eq(state.D0, regs.D0.LW);
	check_eq(state.D1, regs.D1.LW);
	check_eq(state.D2, regs.D2.LW);
	check_eq(state.D3, regs.D3.LW);
	check_eq(state.D4, regs.D4.LW);
	check_eq(state.D5, regs.D5.LW);
	check_eq(state.D6, regs.D6.LW);
	check_eq(state.D7, regs.D7.LW);

	check_eq(state.A0, regs.A0.LW);
	check_eq(state.A1, regs.A1.LW);
	check_eq(state.A2, regs.A2.LW);
	check_eq(state.A3, regs.A3.LW);
	check_eq(state.A4, regs.A4.LW);
	check_eq(state.A5, regs.A5.LW);
	check_eq(state.A6, regs.A6.LW);

	check_eq(state.USP, regs.USP.LW);
	// ASSERT_EQ(state.SR, state.SR);
	check_eq(state.PC, regs.PC);

	// check ram
	auto& mem = cpu.memory();
	for(const auto& ram : state.ram)
	{
		auto actual = mem.read<std::uint8_t>(ram.addres);
		check_eq(ram.value, actual) << "RAM assert failed at " << ram.addres;
	}

	#undef check_eq

	return !failed;
}

bool run_test(m68k::cpu& cpu, const test_case& test)
{
	set_preconditions(cpu, test.initial_state);

	cpu.execute_one();
	
	bool succeded = check_postconditions(cpu, test.final_state);

	return succeded;
}

void run_tests(m68k::cpu& cpu, const std::vector<test_case>& tests, std::string_view test_name)
{
	ASSERT_FALSE(tests.empty()) << test_name << ": tests cannot be empty";

	for(const auto& test : tests)
	{
		std::cout << "Running " << test.name << std::endl;
		bool succeded = run_test(cpu, test);
		ASSERT_TRUE(succeded) << test_name << ": " << test.name << " - failed";
	}

	std::cout << test_name << ": succeded " << tests.size() << std::endl;
}

void load_and_run(std::string test_path)
{
	auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();

	auto tests = load_tests(test_path);
	std::cout << test_name << ": loaded " << tests.size() << " tests" << std::endl;

	auto cpu = m68k::cpu(std::make_shared<m68k::memory>());
	run_tests(cpu, tests, test_name);
}

TEST(M68K, ADD)
{
	const std::string path = R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\ADD.b.json\ADD.b.json)";
	load_and_run(path);
}
