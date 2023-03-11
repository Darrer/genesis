#include <gtest/gtest.h>
#include <iostream>
#include <chrono>

#include "tests_loader.h"
#include "m68k/cpu.h"
#include "m68k/impl/prefetch_queue.hpp"

using namespace genesis;


void set_preconditions(m68k::cpu& cpu, const cpu_state& state)
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
	cpu.prefetch_queue().IR = cpu.prefetch_queue().IRD = state.prefetch.at(0);
	cpu.prefetch_queue().IRC = state.prefetch.at(1);
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
		auto actual = mem.read<std::uint8_t>(ram.address);
		check_eq(ram.value, actual) << "RAM assert failed at " << ram.address;
	}

	// check prefetch queue
	auto& pq = cpu.prefetch_queue();

	check_eq(state.prefetch.at(0), pq.IR);
	check_eq(state.prefetch.at(0), pq.IRD);
	check_eq(state.prefetch.at(1), pq.IRC);

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
		// ss << ", " << (int)rw.func_code; // TODO
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

void execute(m68k::cpu& cpu, std::uint16_t cycles, std::function<void()> on_cycle)
{
	const std::uint16_t bonus_cycles = 10;
	bool in_bonus_cycles = false;

	while (cycles > 0)
	{
		cpu.cycle();
		--cycles;
		
		if(on_cycle != nullptr)
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

run_result execute_and_track(m68k::cpu& cpu, std::uint16_t cycles)
{
	using namespace m68k;

	run_result res;

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
		if(!busm.is_idle() && cycles_in_curr_trans == 3)
		{
			// most of the buses are set on 3rd cycle, so save required data here
			type = bus.is_set(bus::RW) ? trans_type::READ : trans_type::WRITE;
			rw_trans.address = bus.address();
			rw_trans.word_access = bus.is_set(bus::UDS) && bus.is_set(bus::LDS);
			rw_trans.func_code = 0; // TODO

			if(rw_trans.word_access)
				rw_trans.data = bus.data();
			else if(bus.is_set(bus::UDS))
				rw_trans.data = bus.data() >> 8;
			else
				rw_trans.data = bus.data() & 0xFF;
		}


		// transition bus cycle -> idle
		if(busm.is_idle() && in_bus_cycle)
		{
			// push bus transition, assume data was saved
			res.transitions.push_back({type, cycles_in_curr_trans, rw_trans});
			in_bus_cycle = false;
			cycles_in_curr_trans = 0;
		}

		// transition idle -> bus cycle
		if(!busm.is_idle() && !in_bus_cycle)
		{
			// push idle transition
			--cycles_in_curr_trans; // current cycle was a bus cycle, so need to substruct 1 before push
			if(cycles_in_curr_trans != 0)
				res.transitions.push_back({cycles_in_curr_trans});

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

	return res;
}

bool run_test(m68k::cpu& cpu, const test_case& test)
{
	set_preconditions(cpu, test.initial_state);

	auto res = execute_and_track(cpu, test.length);

	bool post = check_postconditions(cpu, test.final_state);
	bool trans = check_transitions(res, test.transitions, test.length);

	return post && trans;
}

void run_tests(m68k::cpu& cpu, const std::vector<test_case>& tests, std::string_view test_name)
{
	ASSERT_FALSE(tests.empty()) << test_name << ": tests cannot be empty";

	std::uint32_t total_cycles = 0;
	auto start = std::chrono::high_resolution_clock::now();

	std::uint32_t num_succeded = 0;

	const std::uint32_t skip_limit = tests.size();
	std::uint32_t skipped = 0;
	for(const auto& test : tests)
	{
		if(test.length >= 50 && skipped < skip_limit)
		{
			// std::cout << "Skipping " << test.name << std::endl;
			++skipped;
			continue;
		}

		// std::cout << "Running " << test.name << std::endl;
		bool succeded = run_test(cpu, test);
		total_cycles += test.length;

		if(succeded)
			++num_succeded;

		ASSERT_TRUE(succeded) << test_name << ": " << test.name << " - failed";
	}

	if(num_succeded == tests.size())
	{
		auto stop = std::chrono::high_resolution_clock::now();
		auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);

		const auto clock_rate = 16.67 /* MHz */ * 1'000'000;
		auto cycle_time_threshold_ns = 1'000'000'000 / clock_rate;

		// as we're measuring not only cycles, but also tests-related code
		// increase threshold value to account for that
		cycle_time_threshold_ns *= 2; // arbitrary chosen

		const auto ns_per_cycle = dur.count() / total_cycles;

		ASSERT_LT(ns_per_cycle, cycle_time_threshold_ns);

		std::cout << "total cycles: " << total_cycles << ", took " << dur.count() << " ns " << std::endl;
		std::cout << "Ns per cycle: " << ns_per_cycle << ", threshold:" << cycle_time_threshold_ns << std::endl;
	}

	ASSERT_EQ(tests.size(), num_succeded);
}

void load_and_run(std::string test_path)
{
	auto test_name = ::testing::UnitTest::GetInstance()->current_test_info()->name();

	auto tests = load_tests(test_path);
	std::cout << test_name << ": loaded " << tests.size() << " tests" << std::endl;

	auto cpu = m68k::make_cpu();
	run_tests(cpu, tests, test_name);
}

TEST(M68K, ADD)
{
	const std::string path = R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\ADD.b.json\ADD.b.json)";
	load_and_run(path);
}

TEST(M68K, ADDW)
{
	const std::string path = R"(C:\Users\darre\Desktop\repo\genesis\tests\m68k\exercisers\ADD.b.json\ADD.w.json)";
	load_and_run(path);
}
