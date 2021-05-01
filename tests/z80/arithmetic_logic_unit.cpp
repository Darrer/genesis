#include "z80/cpu.h"

#include <cstdint>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>

const std::uint16_t reserved = 0x100;

using verify_cb = std::function<bool()>;


void execute(genesis::z80::cpu& cpu, std::initializer_list<std::uint8_t> opcode, verify_cb verify_fn = nullptr)
{
	if (opcode.size() >= reserved)
	{
		throw std::runtime_error("Internal error, only 256 bytes reserved for opcode!");
	}

	// write opcode into memory
	auto& mem = cpu.memory();
	std::uint16_t offset = 0x0;
	for (auto i : opcode)
		mem.write(offset++, i);

	// set up PC
	cpu.registers().PC = 0x0;

	cpu.execute_one();

	if (verify_fn)
		verify_fn();
}


TEST(Z80ArithmeticLogicUnit, ADD)
{
	auto mem = std::make_shared<genesis::z80::z80_mem>();
	auto cpu = genesis::z80::cpu(mem);
	auto& regs = cpu.registers();

	{
		regs.main_set.A = 0x13;
		regs.main_set.HL = reserved + 0x20;

		mem->write<std::uint8_t>(regs.main_set.HL, 0x31);

		execute(cpu, {0x86});
		ASSERT_EQ(regs.main_set.A, 0x13 + 0x31);
	}
}
