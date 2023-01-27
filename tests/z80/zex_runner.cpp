#include <string_view>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "z80/cpu.h"
#include "string_utils.hpp"

using namespace genesis;

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

	mem.write(0x10, 0xC9); // RET
}

void log_pc(z80::cpu& cpu)
{
	auto pc = cpu.registers().PC;
	auto op1 = cpu.memory().read<std::uint8_t>(pc);
	auto op2 = cpu.memory().read<std::uint8_t>(pc + 1);

	std::cout << "PC: " << su::hex_str(pc) << ' ';
	std::cout << "(" << su::hex_str(op1) << ", " << su::hex_str(op2) << ")" << std::endl;
}

void run_test(const std::string& test_path)
{
	auto cpu = z80::cpu(std::make_shared<z80::memory>());

	const std::uint16_t start_address = 0x8000;

	load_at(cpu.memory(), start_address, test_path);
	patch_zex(cpu.memory());

	cpu.registers().PC = start_address;

	while(true)
	{
		log_pc(cpu);
		cpu.execute_one();
	}
}

TEST(Z80, ZEXDOC)
{
	run_test("C:\\Users\\darre\\Desktop\\repo\\genesis\\tests\\z80\\zex\\zexdoc");
}