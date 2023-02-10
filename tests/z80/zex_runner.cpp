#include <string_view>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "../helper.hpp"

#include "z80/cpu.h"
#include "z80/io_ports.hpp"
#include "string_utils.hpp"

using namespace genesis;

class test_io_ports : public z80::io_ports
{
public:
	void out(std::uint8_t /*dev*/, std::uint8_t data) override
	{
		if(data == 13)
			data = '\n';

		if(data == '\n')
			parse_line();

		one_line << data;
		
		check_terminated();
		if(data == '\n')
		{
			std::cout << one_line.str();
			one_line.str(std::string());
		}
	}

	bool terminated()
	{
		return _terminated;
	}

	int succeded() { return num_succeded; }
	int failed() { return num_failed; }


private:
	void check_terminated()
	{
		if(one_line.str().starts_with("Tests complete"))
			_terminated = true;
	}

	void parse_line()
	{
		std::string str = one_line.str();
		
		if(str.ends_with("OK"))
			++num_succeded;
		
		if(str.find("expected") != std::string::npos)
			++num_failed;
	}

private:
	std::stringstream one_line;
	bool _terminated = false;
	int num_succeded = 0;
	int num_failed = 0;
};


void load_at(z80::memory& mem, z80::memory::address base, const std::string& bin_path)
{
	std::ifstream fs(bin_path, std::ios_base::binary);
	if (!fs.is_open())
	{
		throw std::runtime_error("failed to open file '" + bin_path + "'");
	}

	z80::memory::address offset = 0x0;
	while (fs)
	{
		char c;
		if (fs.get(c))
		{
			mem.write(base + offset, c);
			++offset;
		}
	}
}

void patch_zex(z80::memory& mem)
{
	mem.write(0x1601, 0xC9); // RET

	mem.write(0x10, 0x01D3); // OUT (1), A
	mem.write(0x12, 0xC9);   // RET
}

void report_results(int cycles, int num_succeded, int num_failed)
{
	std::cout << "Z80 Test complete, succeeded: " << num_succeded << ", failed: " << num_failed
		<< ", cycles: " << cycles << std::endl;
}

void run_test(const std::string& test_path)
{
	auto ports = std::make_shared<test_io_ports>();
	auto cpu = z80::cpu(std::make_shared<z80::memory>(), ports);

	const std::uint16_t start_address = 0x8000;

	load_at(cpu.memory(), start_address, test_path);
	patch_zex(cpu.memory());

	cpu.registers().PC = start_address;
	unsigned long long total = 0;

	while(!ports->terminated())
	{
		try
		{
			cpu.execute_one();
			++total;
		}
		catch(...)
		{
			report_results(total, ports->succeded(), ports->failed());
			throw;
		}
	}

	report_results(total, ports->succeded(), ports->failed());

	const int total_tests = 67;
	ASSERT_EQ(0, ports->failed()) << "Some tests were failed!";
	ASSERT_EQ(total_tests, ports->succeded()) << "Some tests were skipped";
}

TEST(Z80, ZEXDOC)
{
	auto bin_path = get_exec_path() / "z80" / "zexdoc";
	run_test(bin_path.string());
}

TEST(Z80, ZEXALL)
{
	auto bin_path = get_exec_path() / "z80" / "zexall";
	run_test(bin_path.string());
}
