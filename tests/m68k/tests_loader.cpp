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
	
	if(pref.size() != 2)
		throw std::runtime_error("parse_prefetch error: unexpected prefetch queue size");

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

std::vector<bus_transition> parse_transitions(json& transitions)
{
	if(!transitions.is_array() || transitions.size() == 0)
		throw std::runtime_error("parse_transitions error: expected to get an non-empty array of transitions");

	auto get_trans_type = [](std::string type)
	{
		switch (type.at(0))
		{
		case 'n': return trans_type::IDLE;
		case 'r': return trans_type::READ;
		case 'w': return trans_type::WRITE;
		case 't': return trans_type::READ_MODIFY_WRITE;
		default:
			throw std::runtime_error("parse_transitions error: unknown transition type: " + type);
		}
	};

	std::vector<bus_transition> res;
	for(auto& tr : transitions)
	{
		if(!tr.is_array() || tr.size() == 0)
			throw std::runtime_error("parse_transitions error: found invalid transition");
		
		auto type = get_trans_type(tr.at(0).get<std::string>());
		std::uint16_t cycles = tr.at(1).get<std::uint16_t>();

		switch (type)
		{
		case trans_type::IDLE:
		{
			res.push_back({idle_transition{cycles}});
			break;
		}

		case trans_type::READ:
		case trans_type::WRITE:
		case trans_type::READ_MODIFY_WRITE:
		{
			std::uint8_t func_code = tr.at(2).get<std::uint8_t>();
			std::uint32_t addr = tr.at(3).get<std::uint32_t>();

			std::string access_type = tr.at(4).get<std::string>();
			if(access_type != ".w" && access_type != ".b")
				throw std::runtime_error("parse_transitions error: unknown access type: " + access_type);

			bool word_access = access_type == ".w";
			std::uint16_t data = tr.at(5).get<std::uint16_t>();

			rw_transition rw{cycles, func_code, addr, word_access, data};
			res.push_back({type, rw});

			break;
		}
		}
	}

	res.shrink_to_fit();
	return res;
}

test_case parse_test_case(json& test)
{
	test_case tc;

	tc.name = test["name"].get<std::string>();
	tc.length = test["length"].get<std::uint16_t>();

	tc.initial_state = parse_cpu_state(test["initial"]);
	tc.final_state = parse_cpu_state(test["final"]);
	tc.transitions = parse_transitions(test["transactions"]);

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
