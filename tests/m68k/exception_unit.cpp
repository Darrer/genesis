#include "exception.hpp"
#include "helpers/random.h"
#include "m68k/impl/exception_manager.h"
#include "test_cpu.hpp"
#include "test_program.h"

#include <gtest/gtest.h>
#include <iostream>
#include <string_utils.hpp>

// TODO: add test cases with rising exception during processing another exception


using namespace genesis::m68k;
using namespace genesis::test;

void clean_vector_table(genesis::memory::memory_unit& mem)
{
	for(int addr = 0; addr <= 1024; ++addr)
		mem.write(addr, std::uint8_t(0));
}

void rise_interrupt(test_cpu& cpu, std::uint8_t priority = 1, std::uint8_t IPM = 0)
{
	auto& regs = cpu.registers();

	regs.SSP.LW = 2048;
	regs.flags.IPM = IPM;

	cpu.set_interrupt(priority);
}

void rise_exception(test_cpu& cpu, exception_type ex)
{
	exception_manager& exman = cpu.exception_manager();
	switch(ex)
	{
	case exception_type::address_error:
		exman.rise_address_error({0, 0, false, false});
		break;

	case exception_type::bus_error:
		exman.rise_bus_error({0, 0, false, false});
		break;

	case exception_type::trace:
		exman.rise_trace();
		break;

	case exception_type::interrupt: {
		rise_interrupt(cpu);
		exman.rise_interrupt(cpu.bus().interrupt_priority());
		break;
	}

	default:
		exman.rise(ex);
		break;
	}
}

void setup_interrupt(test_cpu& cpu, std::uint8_t priority = 1, std::uint8_t IPM = 0)
{
	clean_vector_table(cpu.memory());
	rise_interrupt(cpu, priority, IPM);
}

void check_timings(exception_type ex, std::uint8_t expected_cycles)
{
	genesis::test::test_cpu cpu;
	auto& exman = cpu.exception_manager();

	clean_vector_table(cpu.memory());
	cpu.registers().SSP.LW = 2048;

	rise_exception(cpu, ex);

	ASSERT_TRUE(exman.is_raised(ex));

	auto cycles = cpu.cycle_till_idle();

	ASSERT_FALSE(exman.is_raised(ex));
	ASSERT_FALSE(exman.is_raised_any());
	ASSERT_EQ(expected_cycles, cycles);
}


TEST(M68K_EXCEPTION_UNIT, RESET_EXCEPTION_TIMINGS)
{
	check_timings(exception_type::reset, 40);
}

TEST(M68K_EXCEPTION_UNIT, ADDRESS_ERROR_TIMINGS)
{
	// TODO: why 50 - 1?
	check_timings(exception_type::address_error, 50 - 1);
}

TEST(M68K_EXCEPTION_UNIT, BUS_ERROR_TIMINGS)
{
	// TODO: why 50 - 1?
	check_timings(exception_type::bus_error, 50 - 1);
}

TEST(M68K_EXCEPTION_UNIT, TRACE_TIMINGS)
{
	check_timings(exception_type::trace, 34);
}

TEST(M68K_EXCEPTION_UNIT, ILLIGAL_TIMINGS)
{
	check_timings(exception_type::illegal_instruction, 34);
	check_timings(exception_type::line_1010_emulator, 34);
	check_timings(exception_type::line_1111_emulator, 34);
}

const std::uint8_t interrupt_cycles = 44;

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_TIMINGS)
{
	check_timings(exception_type::interrupt, interrupt_cycles);
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_VALIDATE_STACK)
{
	// Setup
	test_cpu cpu;
	auto& regs = cpu.registers();
	auto& mem = cpu.memory();

	const std::uint8_t priority = (random::next<std::uint8_t>() % 7) + 1;
	setup_interrupt(cpu, priority);

	const std::uint32_t initial_sp = 2048;
	regs.SSP.LW = initial_sp;

	regs.SR = random::next<std::uint16_t>();
	regs.flags.IPM = 0;
	const std::uint16_t initial_sr = regs.SR;

	const std::uint32_t initial_pc = random::next<std::uint32_t>();
	regs.PC = initial_pc;

	// Act
	auto cycles = cpu.cycle_till_idle();
	ASSERT_EQ(cycles, interrupt_cycles);

	// Assert
	const std::uint32_t expected_sp = initial_sp - 2 /* SR */ - 4 /* PC */;
	ASSERT_EQ(expected_sp, regs.SSP.LW);

	const std::uint16_t pushed_sr = mem.read<std::uint16_t>(initial_sp - 6);
	ASSERT_EQ(initial_sr, pushed_sr);

	const std::uint32_t pushed_pc = mem.read<std::uint32_t>(initial_sp - 4);
	EXPECT_EQ(initial_pc, pushed_pc);

	// check status flag
	ASSERT_EQ(priority, regs.flags.IPM);
	ASSERT_EQ(1, regs.flags.S);
	ASSERT_EQ(0, regs.flags.TR);
}

void prepare_vector_table(test_cpu& cpu, std::uint32_t address, std::uint32_t pc_value, std::uint16_t ird,
						  std::uint16_t irc)
{
	auto& mem = cpu.memory();

	clean_vector_table(mem);

	mem.write(address, pc_value);
	mem.write(pc_value, ird);
	mem.write(pc_value + 2, irc);
}

std::uint32_t random_pc(test_cpu& cpu)
{
	// PC should not point to vector table
	std::uint32_t min_address = 1024;

	// there should be at least 4 bytes for prefetch queue (IRD/IRC)
	std::uint32_t max_address = cpu.memory().max_address() - 4;

	std::uint32_t pc = random::in_range<std::uint32_t>(min_address, max_address);

	if(pc % 2 == 1)
	{
		// decrement will always keep PC in range [min_address; max_address]
		// as min_address is even and minimum odd value would be min_address + 1
		--pc; // PC must be even
	}

	return pc;
}

std::vector<std::uint32_t> build_interrupt_vector_addresses()
{
	std::vector<std::uint32_t> vectors;

	vectors.push_back(60); // Uninitialized Interrupt Vector
	vectors.push_back(96); // Spurious Interrupt

	// Autovectors
	for(std::uint32_t vec = 100; vec <= 124; vec += 4)
		vectors.push_back(vec);

	// User vectors
	for(std::uint32_t vec = 256; vec <= 1020; vec += 4)
		vectors.push_back(vec);

	return vectors;
}

std::uint8_t setup_int_device(test_cpu& cpu, std::uint32_t vector_addr)
{
	auto& dev = cpu.interrupt_dev();

	std::uint8_t priority = 1;

	if(vector_addr == 60)
	{
		dev.set_uninitialized();
	}
	else if(vector_addr == 96)
	{
		dev.set_spurious();
	}
	else if(vector_addr >= 100 && vector_addr <= 124)
	{
		priority = (vector_addr - 100) / 4 + 1;
		dev.set_autovectored();
	}
	else if(vector_addr >= 256 && vector_addr <= 1024)
	{
		dev.set_vectored(vector_addr / 4);
	}
	else
	{
		throw std::invalid_argument("vector_addr");
	}

	return priority;
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_VECTORS_VALIDATE_PC)
{
	// Setup
	test_cpu cpu;
	auto& regs = cpu.registers();

	auto vec_addresses = build_interrupt_vector_addresses();
	for(auto addr : vec_addresses)
	{
		const std::uint32_t expected_pc = random_pc(cpu);
		const std::uint16_t expected_ird = random::next<std::uint16_t>();
		const std::uint16_t expected_irc = random::next<std::uint16_t>();

		prepare_vector_table(cpu, addr, expected_pc, expected_ird, expected_irc);

		auto priority = setup_int_device(cpu, addr);
		rise_interrupt(cpu, priority);

		// Act
		auto cycles = cpu.cycle_till_idle();
		ASSERT_EQ(cycles, interrupt_cycles);

		// Assert
		ASSERT_EQ(expected_pc, regs.PC);
		ASSERT_EQ(expected_ird, regs.IRD);
		ASSERT_EQ(expected_irc, regs.IRC);
	}
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_UNMASKED)
{
	// Setup
	test_cpu cpu;
	setup_interrupt(cpu, 0b111, 0b111);

	// Act
	auto cycles = cpu.cycle_till_idle();

	// Assert
	ASSERT_EQ(interrupt_cycles, cycles);
	ASSERT_FALSE(cpu.exception_manager().is_raised(exception_type::interrupt));
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_MASKED)
{
	for(std::uint8_t priority = 1; priority <= 5; ++priority)
	{
		// Setup
		test_cpu cpu;
		auto& regs = cpu.registers();

		setup_interrupt(cpu, priority, 5);

		regs.PC = 1024;
		regs.IRC = regs.IRD = nop_opcode;

		// Act
		auto cycles = cpu.cycle_till_idle();

		// Assert
		ASSERT_EQ(4, cycles); // it takes 4 cycles to execute NOP
		ASSERT_FALSE(cpu.exception_manager().is_raised(exception_type::interrupt));
	}
}

/* Interrupt routine:
 * NOP
 * NOP
 * INC <counter_address>
 * RTE // return from exception
 */
void dump_interrupt_routine(test_cpu& cpu, std::uint32_t& base_address, std::uint32_t counter_address)
{
	auto& mem = cpu.memory();

	mem.write(base_address, nop_opcode);
	base_address += 2;

	mem.write(base_address, nop_opcode);
	base_address += 2;

	// ADDQ command
	// data = 1
	// size = LONG
	// ea = abs_long
	const std::uint16_t inc_opcode = 0b0101001010111001;
	mem.write(base_address, inc_opcode);
	base_address += 2;

	mem.write(base_address, counter_address);
	base_address += 4;

	const std::uint16_t rte_opcode = 0b0100111001110011; // RTE - return from exception
	mem.write(base_address, rte_opcode);
	base_address += 2;
}

TEST(M68K_EXCEPTION_UNIT, INTERRUPT_DURING_PROGRAM_EXECUTION)
{
	test_cpu cpu;
	auto& mem = cpu.memory();
	auto& regs = cpu.registers();

	auto interrupt_vectors = build_interrupt_vector_addresses();
	std::map<std::uint32_t /* vector address*/, std::uint32_t /* counter address */> counter_address;
	std::map<std::uint32_t /* vector address */, std::uint32_t /* interrupt routine address */> routine_address;
	std::map<std::uint32_t /* vector address */, std::uint32_t /* num interrupts */> num_interrupts;

	// test program may set IPM to 7 (disable all interrupts) or may use interrupt vector entries to store some data,
	// so we have to save & overwrite some regs/mem to effectively run this test
	std::uint32_t old_vec_addr = 0x0;
	std::uint8_t old_ipm = 0x0;

	// test program set SSP too close to the vector table, so pusing data on the stack may overwrite interrupt vectors
	std::uint32_t test_ssp = 0x0;
	std::uint32_t old_ssp = 0x0;

	std::uint32_t return_pc = 0x0;
	std::uint32_t vec_addr = 0;

	auto get_interrupt_frequency = []() { return random::in_range<std::uint32_t>(100, 500); };

	auto interrupt_frequency = get_interrupt_frequency();

	auto cycles = 0ull;

	enum class test_state
	{
		init,
		wait,
		wait_idle,
		start,
		finish
	};
	test_state state = test_state::init;

	bool success = run_test_program(cpu, [&]() {
		switch(state)
		{
		case test_state::init: {
			// allocate 4 bytes for counter variable for each interrupt handler
			std::uint32_t counter_addr = address_after_test_programm;
			for(auto vec : interrupt_vectors)
			{
				counter_address[vec] = counter_addr;
				mem.write<std::uint32_t>(counter_addr, 0);
				counter_addr += 4;
			}

			// setup interrupt handlers
			std::uint32_t int_routine_addr = counter_addr;
			for(auto vec : interrupt_vectors)
			{
				routine_address[vec] = int_routine_addr;
				dump_interrupt_routine(cpu, int_routine_addr, counter_address[vec]);
			}

			test_ssp = int_routine_addr + 1024;
			state = test_state::wait;
			break;
		}

		case test_state::wait:
			++cycles;
			if((cycles % interrupt_frequency) == 0)
			{
				// start test
				state = test_state::wait_idle;
			}

			break;

		case test_state::wait_idle:
			if(!cpu.is_idle() || cpu.exception_manager().is_raised_any())
				break;

			state = test_state::start;
			[[fallthrough]];

		case test_state::start: {
			ASSERT_TRUE(cpu.is_idle());

			// there should be no other interrupt/exceptions pending
			ASSERT_FALSE(cpu.exception_manager().is_raised_any());
			ASSERT_TRUE(cpu.bus().interrupt_priority() == 0);

			// TODO: may not work if we pick 124 two times in a row (m68k should process only first interrupt)
			vec_addr = random::pick(interrupt_vectors);

			// save old state
			old_vec_addr = mem.read<std::uint32_t>(vec_addr);
			old_ipm = regs.flags.IPM;
			old_ssp = regs.SSP.LW;
			return_pc = regs.PC;

			// overwrite
			mem.write(vec_addr, routine_address.at(vec_addr));
			regs.SSP.LW = test_ssp;
			regs.flags.IPM = 0;

			auto priority = setup_int_device(cpu, vec_addr);
			cpu.set_interrupt(priority);

			if(!num_interrupts.contains(vec_addr))
				num_interrupts[vec_addr] = 0;
			++num_interrupts[vec_addr];

			state = test_state::finish;

			break;
		}

		case test_state::finish: {
			if(cpu.is_idle() && regs.PC == return_pc)
			{
				// restore state
				mem.write(vec_addr, old_vec_addr);
				regs.SSP.LW = old_ssp;
				regs.flags.IPM = old_ipm;

				interrupt_frequency = get_interrupt_frequency();
				state = test_state::wait;
			}

			break;
		}

		default:
			throw genesis::internal_error();
		}
	});

	// wait for last interrupt to complete (if any)
	if(state == test_state::finish)
	{
		cpu.cycle_until([&cpu, return_pc]() { return cpu.is_idle() && cpu.registers().PC == return_pc; });
	}

	ASSERT_TRUE(success);

	{
		constexpr bool enable_logging = false;

		for(auto vec : interrupt_vectors)
		{
			auto counter_value = mem.read<std::uint32_t>(counter_address[vec]);
			auto num_calls = num_interrupts[vec];

			EXPECT_EQ(num_calls, counter_value) << "failed for vector " << vec;

			if(enable_logging)
			{
				std::cout << su::hex_str(vec) << " int counter: " << counter_value << " int called: " << num_calls
						  << '\n';
			}
		}
	}
}

TEST(M68K_EXCEPTION_UNIT, MULTIPLE_ADDRESS_ERRORS)
{
	test_cpu cpu;
	auto& mem = cpu.memory();

	rise_exception(cpu, exception_type::address_error);

	// 0xC points to address error routine, setting it to odd address will lead to rising
	// address error exception during processing address error exception
	mem.write<std::uint32_t>(0xC, 0x1);

	ASSERT_THROW(cpu.cycle_until([&]() { return cpu.is_idle(); }), std::runtime_error);
}

TEST(M68K_EXCEPTION_UNIT, BUS_ADDRESS_ERROR_VALIDATE_STACK)
{
	// Setup
	test_cpu cpu;
	auto& exman = cpu.exception_manager();
	auto& regs = cpu.registers();
	auto& mem = cpu.memory();

	// prepare exception data
	exception_type exp = random::pick({exception_type::bus_error, exception_type::address_error});

	std::uint32_t address = random::next<std::uint32_t>();
	std::uint8_t func_codes = random::in_range<std::uint8_t>(0b000, 0b111);
	bool rw = random::is_true();
	bool in = random::is_true();

	std::uint32_t table_pc = random_pc(cpu);

	clean_vector_table(mem);
	if(exp == exception_type::bus_error)
	{
		exman.rise_bus_error({address, func_codes, rw, in});
		mem.write<std::uint32_t>(0x8, table_pc);
	}
	else
	{
		exman.rise_address_error({address, func_codes, rw, in});
		mem.write<std::uint32_t>(0xC, table_pc);
	}

	// prepare registers/memory
	const std::uint32_t initial_sp = 2048;
	regs.SSP.LW = initial_sp;

	regs.SR = random::next<std::uint16_t>();
	regs.flags.IPM = 0;
	const std::uint16_t initial_sr = regs.SR;

	const std::uint32_t initial_pc = random_pc(cpu);
	regs.PC = initial_pc;

	const std::uint16_t initial_sird = random::next<std::uint16_t>();
	regs.SIRD = initial_sird; // assume SIRD is correctly copied from IRD

	// Act
	cpu.cycle_till_idle();

	// Assert
	ASSERT_FALSE(exman.is_raised(exp));

	const std::uint32_t pushed_pc = mem.read<std::uint32_t>(initial_sp - 4);
	// as PC is corrected during pushing, we cannot simply compare it, make sure it's just 'close enough'
	EXPECT_LT(initial_pc - pushed_pc, 10); // assume pushed PC cannot be less than 10 bytes from the initial one.

	const std::uint16_t pushed_sr = mem.read<std::uint16_t>(initial_sp - 6);
	ASSERT_EQ(initial_sr, pushed_sr);

	const std::uint16_t pushed_ird = mem.read<std::uint16_t>(initial_sp - 8);
	ASSERT_EQ(pushed_ird, initial_sird);

	const std::uint32_t pushed_adrr = mem.read<std::uint32_t>(initial_sp - 12);
	ASSERT_EQ(pushed_adrr, address);

	const std::uint16_t pushed_status = mem.read<std::uint16_t>(initial_sp - 14);
	std::uint16_t expected_status = func_codes;
	expected_status |= (in ? 1 : 0) << 3;
	expected_status |= (rw ? 1 : 0) << 4;
	ASSERT_EQ(pushed_status & 0b11111, expected_status); // check only documented the first 5 bits

	const std::uint32_t expected_sp =
		initial_sp - 4 /* PC */ - 2 /* SR */ - 2 /* ISR */ - 4 /* ADDR */ - 2 /* status word */;
	ASSERT_EQ(expected_sp, regs.SSP.LW);

	ASSERT_EQ(table_pc, regs.PC);
}
