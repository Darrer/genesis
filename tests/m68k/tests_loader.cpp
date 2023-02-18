#include "tests_loader.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <iostream>


using json = nlohmann::json;

ram_state parse_ram_state(json& ram)
{
	if(!ram.is_array())
		throw std::runtime_error("parse_ram_state error: expected to get an array");

	ram_state rt;

	for(auto& item : ram)
	{
		if(!item.is_array() || item.size() != 2)
			throw std::runtime_error("parse_ram_state error: expected to get 2 size sub array");
		
		auto addr = item.at(0).get<std::uint32_t>();
		auto value = item.at(1).get<std::uint8_t>();

		rt.push_back({addr, value});
	}

	rt.shrink_to_fit();
	return rt;
}

std::vector<std::uint16_t> parse_prefetch(json& pref)
{
	if(!pref.is_array())
		throw std::runtime_error("parse_prefetch error: expected to get an array");
	
	std::vector<std::uint16_t> prefetch;

	for(auto& val : pref)
		prefetch.push_back(val.get<std::uint16_t>());

	prefetch.shrink_to_fit();
	return prefetch;
}

cpu_state parse_cpu_state(json& state)
{
	cpu_state ct;

	ct.D0 = state["d0"].get<std::uint32_t>();
	ct.D1 = state["d1"].get<std::uint32_t>();
	ct.D2 = state["d2"].get<std::uint32_t>();
	ct.D3 = state["d3"].get<std::uint32_t>();
	ct.D4 = state["d4"].get<std::uint32_t>();
	ct.D5 = state["d5"].get<std::uint32_t>();
	ct.D6 = state["d6"].get<std::uint32_t>();
	ct.D7 = state["d7"].get<std::uint32_t>();

	ct.A0 = state["a0"].get<std::uint32_t>();
	ct.A1 = state["a1"].get<std::uint32_t>();
	ct.A2 = state["a2"].get<std::uint32_t>();
	ct.A3 = state["a3"].get<std::uint32_t>();
	ct.A4 = state["a4"].get<std::uint32_t>();
	ct.A5 = state["a5"].get<std::uint32_t>();
	ct.A6 = state["a6"].get<std::uint32_t>();

	ct.USP = state["usp"].get<std::uint32_t>();
	ct.PC = state["pc"].get<std::uint32_t>();
	ct.SR = state["sr"].get<std::uint16_t>();

	ct.ram = parse_ram_state(state["ram"]);
	ct.prefetch = parse_prefetch(state["prefetch"]);

	return ct;
}

test_case parse_test_case(json& test)
{
	test_case tc;

	tc.name = test["name"].get<std::string>();
	tc.length = test["length"].get<std::uint16_t>();

	tc.initial_state = parse_cpu_state(test["initial"]);
	tc.final_state = parse_cpu_state(test["final"]);

	return tc;
}

std::vector<test_case> load_tests(std::string path_to_json_tests)
{
	std::ifstream fs(path_to_json_tests);
	if(!fs.is_open())
		throw std::runtime_error("load_tests error: failed to open " + path_to_json_tests);

	json data = json::parse(fs);

	std::cout << std::boolalpha << "is array: " << data.is_array() << ", size: " << data.size() << std::endl;

	if(!data.is_array())
		return {};

	std::vector<test_case> tests;

	for(auto& test : data)
		tests.push_back(parse_test_case(test));

	tests.shrink_to_fit();
	return tests;
}
