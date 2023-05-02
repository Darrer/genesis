#include <gtest/gtest.h>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <stdlib.h>

#include "tests_loader.h"
#include "test_cpu.hpp"


using namespace genesis;


void set_preconditions(test::test_cpu& cpu, const cpu_state& state)
{
	// setup registers
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
	regs.SSP.LW = state.SSP;
	regs.SR = state.SR;
	regs.PC = state.PC;

	// setup ram
	auto& mem = cpu.memory();
	for(auto [addr, value] : state.ram)
		mem.write(addr, value);

	// setup prefetch queue
	regs.IR = regs.IRD = state.prefetch.at(0);
	regs.IRC = state.prefetch.at(1);
}

bool check_postconditions(test::test_cpu& cpu, const cpu_state& state)
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
	check_eq(state.SSP, regs.SSP.LW);
	check_eq(state.SR, regs.SR);
	check_eq(state.PC, regs.PC);

	// check ram
	auto& mem = cpu.memory();
	for(auto [addr, value] : state.ram)
	{
		auto actual = mem.read<std::uint8_t>(addr);
		check_eq(value, actual) << "RAM assert failed at " << addr;
	}

	// check prefetch queue
	check_eq(state.prefetch.at(0), regs.IR);
	check_eq(state.prefetch.at(0), regs.IRD);
	check_eq(state.prefetch.at(1), regs.IRC);

	#undef check_eq

	return !failed;
}

std::string bus_transition_to_str(const bus_transition& trans)
{
	auto trans_type_str = [](trans_type type) -> std::string
	{
		switch (type)
		{
		case trans_type::IDLE: return "n";
		case trans_type::READ: return "r";
		case trans_type::WRITE: return "w";
		case trans_type::READ_MODIFY_WRITE: return "t";
		default:
			throw std::runtime_error("trans_type_str: unknown trans type");
		}
	};

	std::stringstream ss;
	ss << "[" << trans_type_str(trans.type);
	ss << ", " << (int)trans.cycles;

	if(trans.type != trans_type::IDLE)
	{
		const rw_transition& rw = trans.rw_trans();
		ss << ", " << (int)rw.func_code;
		ss << ", " << (int)rw.address;
		ss << ", " << (rw.word_access ? ".w" : ".b");
		ss << ", " << (int)rw.data;
	}

	ss << "]";
	return ss.str();
}

std::string transitions_to_str(const std::vector<bus_transition>& transitions)
{
	std::stringstream ss;
	ss << "[";

	for(std::size_t i = 0; i < transitions.size(); ++i)
	{
		const auto& trans = transitions[i];
		ss << bus_transition_to_str(trans);

		if((i + 1) != transitions.size())
		{
			ss << ", ";
		}
	}

	ss << "]";
	return ss.str();
}

struct run_result
{
	std::uint32_t cycles = 0;
	std::vector<bus_transition> transitions;
};

bool check_transitions(const run_result& res, const std::vector<bus_transition>& expected_trans, std::uint16_t expected_cycles)
{
	EXPECT_EQ(expected_cycles, res.cycles);

	if(expected_trans != res.transitions)
	{
		auto expected = transitions_to_str(expected_trans);
		auto actual = transitions_to_str(res.transitions);
		EXPECT_EQ(expected, actual);

		return false;
	}

	return res.cycles == expected_cycles;
}

template<class Callable>
void execute(test::test_cpu& cpu, std::uint16_t cycles, Callable on_cycle)
{
	const std::uint16_t bonus_cycles = 10;
	bool in_bonus_cycles = false;

	while (cycles > 0)
	{
		cpu.cycle();
		--cycles;

		on_cycle();

		if(cycles == 0 && !cpu.is_idle() && !in_bonus_cycles)
		{
			// we ate all cycles, but cpu is still doing smth
			// so there is a defenetly issue, add a few bonus cycles
			// just to track bus transitions to report later
			cycles += bonus_cycles;
			in_bonus_cycles = true;
		}

		if(cpu.is_idle() /*&& in_bonus_cycles*/)
		{
			// don't eat extra bonuc cycles
			break;
		}
	}
}

void execute_and_track(test::test_cpu& cpu, std::uint16_t cycles, run_result& res)
{
	using namespace m68k;

	res.cycles = 0;
	res.transitions.clear();

	bool in_bus_cycle = false;
	std::uint16_t cycles_in_curr_trans = 0;
	trans_type type = trans_type::IDLE;
	rw_transition rw_trans;

	auto& busm = cpu.bus_manager();
	auto& bus = cpu.bus();

	auto on_cycle = [&]()
	{
		++res.cycles;
		++cycles_in_curr_trans;

		// save data for rw transition
		if(in_bus_cycle && cycles_in_curr_trans == 3 || cycles_in_curr_trans == 9)
		{
			if(cycles_in_curr_trans == 3)
				type = bus.is_set(bus::RW) ? trans_type::READ : trans_type::WRITE;
			else
				type = trans_type::READ_MODIFY_WRITE;

			rw_trans.address = bus.address();
			rw_trans.word_access = bus.is_set(bus::UDS) && bus.is_set(bus::LDS);
			rw_trans.func_code = bus.func_codes();

			if(rw_trans.word_access)
				rw_trans.data = bus.data();
			else if(bus.is_set(bus::UDS))
				rw_trans.data = bus.data() >> 8;
			else
				rw_trans.data = bus.data() & 0xFF;
		}

		// transition bus cycle -> idle
		if(in_bus_cycle && busm.is_idle())
		{
			res.transitions.push_back({type, cycles_in_curr_trans, rw_trans});
			in_bus_cycle = false;
			cycles_in_curr_trans = 0;
		}

		// transition idle -> bus cycle
		if(!busm.is_idle() && !in_bus_cycle)
		{
			// push idle transition
			if(cycles_in_curr_trans > 1)
			{
				--cycles_in_curr_trans; // current cycle was a bus cycle, so need to substruct 1 before push
				if(cycles_in_curr_trans != 0)
					res.transitions.push_back({cycles_in_curr_trans});
			}

			// start bus cycle
			cycles_in_curr_trans = 1; // 1 as current cycle was a bus cycle
			in_bus_cycle = true;
			rw_trans = rw_transition();
			type = trans_type::IDLE;
		}
	};

	execute(cpu, cycles, on_cycle);

	// push idle cycles
	if(busm.is_idle() && cycles_in_curr_trans > 0)
		res.transitions.push_back({cycles_in_curr_trans});
}

bool run_test(test::test_cpu& cpu, const test_case& test)
{
	set_preconditions(cpu, test.initial_state);

	static run_result res;
	execute_and_track(cpu, test.length, res);

	bool post = check_postconditions(cpu, test.final_state);
	bool trans = check_transitions(res, test.transitions, test.length);

	return post && trans;
}

// using namespace std;
// void * operator new(size_t size)
// {
// 	void * p = malloc(size);
// 	return p;
// }
 
// void operator delete(void * p)
// {
// 	free(p);
// }

bool should_skip_test(std::string_view test_name)
{
	// These are faulty tests
	auto tests_to_skip = { "e502 [ASL.b Q, D2] 1583", "e502 [ASL.b Q, D2] 1761" };
	for(auto& test : tests_to_skip)
	{
		if(test_name == test)
			return true;
	}

	return false;
}

void accept_reset(test::test_cpu& cpu)
{
	auto& exman = cpu.exception_manager();
	if(exman.is_raised(m68k::exception_type::reset))
		exman.accept_reset();
}

bool run_tests(test::test_cpu& cpu, const std::vector<test_case>& tests, std::string_view test_name)
{
	EXPECT_FALSE(tests.empty()) << test_name << ": tests cannot be empty";

	accept_reset(cpu);

	std::uint32_t total_cycles = 0;
	auto start = std::chrono::high_resolution_clock::now();

	std::uint32_t num_succeded = 0;

	const std::uint32_t skip_limit = 2;
	std::uint32_t skipped = 0;
	for(const auto& test : tests)
	{
		if(should_skip_test(test.name))
		{
			std::cout << "Skipping " << test.name << std::endl;
			++skipped;
			continue;
		}

		// std::cout << "Running " << test.name << std::endl;
		bool succeded = run_test(cpu, test);
		total_cycles += test.length;

		if(succeded)
			++num_succeded;

		EXPECT_TRUE(succeded) << test_name << ": " << test.name << " - failed";
		if(!succeded) return false;
	}

	// if(num_succeded == tests.size())
	{
		auto stop = std::chrono::high_resolution_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

		const auto clock_rate = 16.67 /* MHz */ * 1'000'000;
		auto cycle_time_threshold_ns = 1'000'000'000 / clock_rate;

		// as we're measuring not only cycles, but also tests-related code
		// increase threshold value to account for that
		cycle_time_threshold_ns *= 4; // arbitrary chosen

		const auto ns_per_cycle = dur.count() / total_cycles;

		// ASSERT_LT(ns_per_cycle, cycle_time_threshold_ns);

		// std::cout << "total cycles: " << total_cycles << ", took " << dur.count() << " ns " << std::endl;
		std::cout << "Ns per cycle: " << ns_per_cycle << ", threshold:" << cycle_time_threshold_ns << std::endl;
	}

	EXPECT_EQ(tests.size(), num_succeded + skipped);
	EXPECT_TRUE(skipped <= skip_limit);

	return true || num_succeded == tests.size();
}

bool load_and_run(std::string test_path)
{
	std::string test_name = std::filesystem::path(test_path).filename().string();
	su::remove_ch(test_name, '\"');

	auto tests = load_tests(test_path);
	std::cout << test_name << ": executing " << tests.size() << " tests" << std::endl;

	static test::test_cpu cpu;
	return run_tests(cpu, tests, test_name);
}

std::vector<std::string> collect_all_files(std::string dir_path, std::string extension)
{
	if(!extension.starts_with("."))
		extension = "." + extension;

	std::vector<std::string> res;

	namespace fs = std::filesystem;

	std::function<void(std::string)> collect_all;
	collect_all = [&](std::string path)
	{
		for(auto& entry : fs::directory_iterator(path))
		{
			if(entry.is_regular_file())
			{
				auto file = entry.path();
				if(file.extension() == extension)
					res.push_back(file.string());
			}
			if(entry.is_directory())
			{
				collect_all(entry.path().string());
			}
		}
	};

	collect_all(dir_path);

	return res;
}

// Tom Harte's Tests
TEST(M68K, THT)
{
	auto all_tests = collect_all_files(R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\implemented)", "json");
	std::sort(all_tests.begin(), all_tests.end());

	for(auto& test : all_tests)
	{
		bool succeeded = load_and_run(test);
		ASSERT_TRUE(succeeded);
	}
}

// TODO: overload new operator here, make sure it won't be called during executing cpu.cycle()
// as this will be a huge performance hit

// keep it here for debug
TEST(M68K, TMP)
{
	auto all_tests = collect_all_files(R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\TAS)", "json");
	std::sort(all_tests.begin(), all_tests.end());

	for(auto& test : all_tests)
	{
		bool succeeded = load_and_run(test);
		ASSERT_TRUE(succeeded);
	}
}
