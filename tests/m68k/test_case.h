#ifndef __M68K_TEST_CASE_H__
#define __M68K_TEST_CASE_H__

#include <string>
#include <vector>
#include <cstdint>

struct addr_value_pair
{
	std::uint32_t addres;
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
	
	ram_state ram;
};

struct test_case
{
	std::string name;
	cpu_state initial_state;
	cpu_state final_state;
	std::uint16_t length = 0;

	// transactions are not supported
};

#endif // __M68K_TEST_CASE_H__
