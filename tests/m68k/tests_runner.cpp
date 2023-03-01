#include <gtest/gtest.h>
#include <iostream>

#include "tests_loader.h"
#include "m68k/cpu.h"
#include "m68k/impl/prefetch_queue.hpp"


using namespace genesis;

struct run_result
{
	std::uint32_t cycles = 0;
	std::vector<bus_transition> transitions;
};


void set_preconditions(m68k::cpu& cpu, const cpu_state& state, const cpu_state& final)
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
	// regs.SR = state.SR; // skip for now
	regs.PC = state.PC;

	// setup ram
	auto& mem = cpu.memory();
	for(const auto& ram : state.ram)
		mem.write(ram.address, ram.value);

	// setup prefetch queue
	cpu.prefetch_queue().IR = cpu.prefetch_queue().IRD = state.prefetch.at(0);
	cpu.prefetch_queue().IRC = state.prefetch.at(1);
	
	// momory for prefetch queue is specified in final state
	std::uint32_t offset = 0x0;
	for(auto val : final.prefetch)
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
		auto actual = mem.read<std::uint8_t>(ram.address);
		check_eq(ram.value, actual) << "RAM assert failed at " << ram.address;
	}

	// check prefetch queue
	auto& pq = cpu.prefetch_queue();

	check_eq(state.prefetch.at(0), pq.IR);
	check_eq(state.prefetch.at(0), pq.IRD);
	check_eq(state.prefetch.at(1), pq.IRC);

	// TOOD: check cycles
	// TODO: check bus transitions

	#undef check_eq

	return !failed;
}

run_result execute(m68k::cpu& cpu, std::uint16_t cycles)
{
	using namespace m68k;

	run_result res;

	bool in_bus_cycle = false;
	std::uint16_t cycles_in_curr_trans = 0;
	rw_transition rw_trans;

	std::uint16_t bonus_cycles = 10; // +2 bus cycles with 2 extra
	bool in_bonus_cycles = false;

	auto& busm = cpu.bus_manager();
	auto& bus = cpu.bus();
	while (cycles > 0)
	{
		cpu.cycle();
		--cycles;
		++res.cycles;
		
		++cycles_in_curr_trans;
		if(in_bus_cycle)
		{
			if(busm.is_idle())
			{
				// bus transition is over
				res.transitions.push_back({trans_type::READ, cycles_in_curr_trans, rw_trans});

				in_bus_cycle = false;
				cycles_in_curr_trans = 0;
				rw_trans = rw_transition();
			}
			else
			{
				if(cycles_in_curr_trans == 3)
				{
					// most of the buses are set on 3rd cycle, so save required data here
					rw_trans.address = bus.address();
					rw_trans.data = bus.data();
					rw_trans.word_access = bus.is_set(bus::UDS) && bus.is_set(bus::LDS);
					rw_trans.func_code = 0; // TODO

					if(!rw_trans.word_access)
						rw_trans.data = rw_trans.data & 0xFF;
				}
			}
		}
		else
		{
			if(!busm.is_idle())
			{
				// just started bus cycle
				if(cycles_in_curr_trans - 1 != 0)
					res.transitions.push_back({cycles_in_curr_trans - 1});

				cycles_in_curr_trans = 1; // this cycle is the first bus cycle
				in_bus_cycle = true;
				rw_trans = rw_transition();
			}
		}

		if(in_bonus_cycles && busm.is_idle())
		{
			break;
		}

		if(cycles == 0 && !busm.is_idle())
		{
			// we ate all cycles, but bus manager is still doing smth
			// so there is a defenetly issue, add a few bonus cycles
			// just to track bus transitions to report later
			cycles += bonus_cycles;
			bonus_cycles = 0;
			in_bonus_cycles = true;
		}
	}

	return res;
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
	ss << "['" << trans_type_str(trans.type) << "'";
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

std::string transitions_to_str(std::vector<bus_transition> transitions)
{
	std::string res = "[";

	for(std::size_t i = 0; i < transitions.size(); ++i)
	{
		const auto& trans = transitions[i];
		res += bus_transition_to_str(trans);

		if((i + 1) != transitions.size())
		{
			res += ", ";
		}
	}

	res += "]";
	return res;
}

bool check_transitions(const run_result& res, const std::vector<bus_transition>& expected_trans, std::uint16_t expected_cycles)
{
	EXPECT_EQ(expected_cycles, res.cycles);

	auto expected = transitions_to_str(expected_trans);
	auto actual = transitions_to_str(res.transitions);
	EXPECT_EQ(expected, actual);

	return res.cycles == expected_cycles && expected == actual;
}

bool run_test(m68k::cpu& cpu, const test_case& test)
{
	set_preconditions(cpu, test.initial_state, test.final_state);

	auto res = execute(cpu, test.length);
	
	bool succeded = check_postconditions(cpu, test.final_state);
	succeded = succeded && check_transitions(res, test.transitions, test.length);

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
