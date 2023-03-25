#include "../helper.hpp"
#include "string_utils.hpp"
#include "tap_loader.hpp"
#include "z80/cpu.h"
#include "z80/io_ports.hpp"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string_view>

using namespace genesis;

class test_io_ports : public z80::io_ports
{
public:
	std::uint8_t in(std::uint8_t /*dev*/, std::uint8_t /*param*/) override
	{
		return 0xBF;
	}

	void out(std::uint8_t dev, std::uint8_t /*param*/, std::uint8_t data) override
	{
		if (dev != 0x1)
			return;

		data = map_char(data);

		if (!need_to_print(data))
			return;

		if (data == 13)
			data = '\n';

		if (data == '\n')
			parse_line();

		one_line << data;

		check_terminated();
		if (data == '\n')
		{
			std::cout << one_line.str();
			one_line.str(std::string());
		}
	}

	bool terminated()
	{
		return _terminated;
	}

	int succeded()
	{
		return num_succeded;
	}
	int failed()
	{
		return num_failed;
	}


private:
	bool need_to_print(char ch)
	{
		return isprint(ch) || ch == '\n';
	}

	char map_char(char ch)
	{
		switch (ch)
		{
		case '\r':
			return '\n';

		case 30: // record separator
		case 26: // substitute
			return ' ';

		default:
			return ch;
		}
	}

	void check_terminated()
	{
		if (one_line.str().starts_with("Tests complete"))
			_terminated = true;

		if (one_line.str().starts_with("Result:"))
			_terminated = true;
	}

	void parse_line()
	{
		std::string str = one_line.str();
		std::transform(str.cbegin(), str.cend(), str.begin(), [](char ch) { return tolower(ch); });

		if (str.ends_with("ok"))
			++num_succeded;

		if (str.find("expected") != std::string::npos)
			++num_failed;
	}

private:
	std::stringstream one_line;
	bool _terminated = false;
	int num_succeded = 0;
	int num_failed = 0;
};


void report_results(unsigned long long cycles, int num_succeded, int num_failed)
{
	std::cout << "Z80 Test complete, succeeded: " << num_succeded << ", failed: " << num_failed
			  << ", cycles: " << cycles << std::endl;
}

void run_test(z80::cpu& cpu, const int expected_total_tests)
{
	unsigned long long cycles = 0;
	auto& ports = static_cast<test_io_ports&>(cpu.io_ports());

	while (!ports.terminated())
	{
		try
		{
			cpu.execute_one();
			++cycles;
		}
		catch (...)
		{
			report_results(cycles, ports.succeded(), ports.failed());
			throw;
		}
	}

	report_results(cycles, ports.succeded(), ports.failed());

	ASSERT_EQ(0, ports.failed()) << "Some tests were failed!";
	ASSERT_EQ(expected_total_tests, ports.succeded()) << "Some tests were skipped";
}

void patch_mem(z80::memory& mem)
{
	mem.write(0x1601, 0xC9); // RET

	mem.write(0x10, 0x01D3); // OUT (1), A
	mem.write(0x12, 0xC9);	 // RET
}

void load_zex(z80::memory& mem, z80::memory::address base, const std::string& bin_path)
{
	std::ifstream fs(bin_path, std::ios_base::binary);
	if (!fs.is_open())
	{
		throw std::runtime_error("load_zex error: failed to open file '" + bin_path + "'");
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

void run_zex(const std::string& zex_path)
{
	auto cpu = z80::cpu(std::make_shared<z80::memory>(), std::make_shared<test_io_ports>());

	cpu.registers().PC = 0x8000;
	load_zex(cpu.memory(), cpu.registers().PC, zex_path);
	patch_mem(cpu.memory());

	run_test(cpu, 67);
}

void run_tap(const std::string& tap_path)
{
	auto cpu = z80::cpu(std::make_shared<z80::memory>(), std::make_shared<test_io_ports>());

	test::z80::load_tap(tap_path, cpu);
	patch_mem(cpu.memory());

	run_test(cpu, 160);
}


TEST(Z80, ZEXDOC)
{
	auto bin_path = get_exec_path() / "z80" / "zexdoc";
	run_zex(bin_path.string());
}

TEST(Z80, ZEXALL)
{
	auto bin_path = get_exec_path() / "z80" / "zexall";
	run_zex(bin_path.string());
}

TEST(Z80, DOCTAP)
{
	auto tap_path = get_exec_path() / "z80" / "z80doc.tap";
	run_tap(tap_path.string());
}

TEST(Z80, FULLTAP)
{
	auto tap_path = get_exec_path() / "z80" / "z80full.tap";
	run_tap(tap_path.string());
}
