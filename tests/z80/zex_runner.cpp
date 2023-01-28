#include <string_view>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

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
		{
			data = '\n';
		}

		buffer << data;
	}

	std::string data() { return buffer.str(); }

private:
	std::stringstream buffer;
};

void log_pc(z80::cpu& cpu)
{
	auto pc = cpu.registers().PC;
	auto op1 = cpu.memory().read<std::uint8_t>(pc);
	auto op2 = cpu.memory().read<std::uint8_t>(pc + 1);

	std::cout << "PC: " << su::hex_str(pc) << ' ';
	std::cout << "(" << su::hex_str(op1) << ", " << su::hex_str(op2) << ")" << std::endl;
}

void log_io_ports(test_io_ports& ports)
{
	std::cout << std::endl << "=========== <Printed by exerciser:> ===========\n" << std::endl;
	std::cout << ports.data() << std::endl;
	std::cout << std::endl << "==================== <end> ====================" << std::endl;
}

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

void run_test(const std::string& test_path)
{
	auto ports = std::make_shared<test_io_ports>();
	auto cpu = z80::cpu(std::make_shared<z80::memory>(), ports);

	const std::uint16_t start_address = 0x8000;

	load_at(cpu.memory(), start_address, test_path);
	patch_zex(cpu.memory());

	cpu.registers().PC = start_address;

	while(true)
	{
		log_pc(cpu);
		try
		{
			cpu.execute_one();
		}
		catch(...)
		{
			log_io_ports(*ports);
			throw;
		}
	}
}

TEST(Z80, ZEXDOC)
{
	run_test("C:\\Users\\darre\\Desktop\\repo\\genesis\\tests\\z80\\zex\\zexdoc");
}
