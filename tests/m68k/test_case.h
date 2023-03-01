#ifndef __M68K_TEST_CASE_H__
#define __M68K_TEST_CASE_H__

#include <string>
#include <vector>
#include <cstdint>
#include <optional>


enum trans_type : std::uint8_t
{
	IDLE,
	READ,
	WRITE,
	READ_MODIFY_WRITE
};

struct rw_transition
{
	std::uint8_t func_code = 0;
	std::uint32_t address = 0;
	bool word_access = false;
	std::uint16_t data = 0;
};

struct bus_transition
{
	bus_transition(std::uint16_t cycles) : cycles(cycles), type(trans_type::IDLE) { }
	bus_transition(trans_type type, std::uint16_t cycles, rw_transition rw)
		:  cycles(cycles), type(type), rw_data(rw) { }

	const rw_transition& rw_trans() const
	{
		return rw_data.value();
	}

	std::uint16_t cycles = 0;
	trans_type type;

private:
	std::optional<rw_transition> rw_data;
};

struct addr_value_pair
{
	std::uint32_t address;
	std::uint8_t value;
};

using ram_state = std::vector<addr_value_pair>;

struct cpu_state
{
	std::uint32_t D0, D1, D2, D3, D4, D5, D6, D7;
	std::uint32_t A0, A1, A2, A3, A4, A5, A6;

	std::uint32_t USP;
	std::uint16_t SR;
	std::uint32_t PC;
	
	std::vector<std::uint16_t> prefetch;
	ram_state ram;
};

struct test_case
{
	std::string name;
	cpu_state initial_state;
	cpu_state final_state;
	std::uint16_t length = 0;
	std::vector<bus_transition> transitions;
};

#endif // __M68K_TEST_CASE_H__
