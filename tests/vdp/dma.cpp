#include <gtest/gtest.h>
#include <iostream>

#include "test_vdp.h"
#include "../helpers/random.h"

using namespace genesis;
using namespace genesis::vdp;

void zero_mem(vram_t& mem)
{
	for(int addr = 0; addr < mem.max_capacity; ++addr)
		mem.write<std::uint8_t>(addr, 0);
}

std::uint32_t setup_dma(test::vdp& vdp, std::uint32_t address, std::uint16_t length,
	dma_mode mode, vmem_type mem_type, std::uint16_t fill_data)
{
	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();

	sett.dma_length(length);
	sett.dma_mode(mode);
	vdp.registers().R1.M1 = 1; // enable DMA

	control_register control;
	control.address(address);
	control.dma_start(true);
	control.vmem_type(mem_type);
	control.work_completed(false);
	control.control_type(control_type::write); // TODO: does it affect DMA?

	std::uint32_t cycles = 0;

	// write control
	ports.init_write_control(control.raw_c1());
	cycles += vdp.wait_io_ports();

	ports.init_write_control(control.raw_c2());
	cycles += vdp.wait_io_ports();

	// write fill data
	ports.init_write_data(fill_data);
	cycles += vdp.wait_io_ports();

	return cycles;
}

std::uint32_t dma_final_address(std::uint32_t start_address, std::uint16_t length, std::uint8_t auto_inc)
{
	// Address is incremented by:
	// - auto_inc - by first data port write
	// - length * auto_inc - by DMA
	return start_address + auto_inc + (length * auto_inc);
}


TEST(VDP_DMA, START_DMA_WHEN_DMA_DISABLED)
{
	test::vdp vdp;
	auto& regs = vdp.registers();
	auto& ports = vdp.io_ports();

	regs.R1.M1 = 0; // disalbe DMA

	auto random_addr = test::random::next<std::uint16_t>();

	control_register control;
	control.dma_start(true); // set CD5
	control.address(random_addr);

	ports.init_write_control(control.raw_c1());
	vdp.wait_io_ports();

	ports.init_write_control(control.raw_c2());
	vdp.wait_io_ports();

	// make sure write took place
	ASSERT_EQ(random_addr, regs.control.address());

	// CD5 should not be updated
	ASSERT_EQ(false, regs.control.dma_start());
}

TEST(VDP_DMA, BASIC_FILL_VRAM_EVEN_ADDR_AUTO_INC_1)
{
	test::vdp vdp;
	auto& regs = vdp.registers();
	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& mem = vdp.vram();

	const std::uint16_t start_address = 0;
	const std::uint16_t length = 100;
	const std::uint16_t fill_data = 0xDEAD;
	const std::uint8_t fill_msb = endian::msb(fill_data);
	const std::uint8_t fill_lsb = endian::lsb(fill_data);

	zero_mem(mem);

	// prepare DMA
	regs.R15.INC = 1; // set auto inc
	setup_dma(vdp, start_address, length, dma_mode::vram_fill, vmem_type::vram, fill_data);

	// all setup - wait DMA now
	vdp.wait_dma();

	// assert memory

	// first written byte should be LSB
	ASSERT_EQ(fill_lsb, mem.read<std::uint8_t>(start_address));

	// all subsequent written bytes - MSB
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = start_address + 1 + i;
		ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(addr));
	}

	std::uint16_t final_addr = dma_final_address(start_address, length, 1);
	ASSERT_EQ(final_addr, regs.control.address());

	// make sure DMA didn't touch memory after final address
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = final_addr + i;
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr));
	}
}

TEST(VDP_DMA, BASIC_FILL_VRAM_EVEN_ADDR_AUTO_INC_2)
{
	test::vdp vdp;
	auto& regs = vdp.registers();
	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& mem = vdp.vram();

	const std::uint16_t start_address = 0;
	const std::uint16_t length = 100;
	const std::uint16_t fill_data = 0xABCD;
	const std::uint8_t fill_msb = endian::msb(fill_data);

	zero_mem(mem);

	// prepare DMA
	regs.R15.INC = 2; // set auto inc
	setup_dma(vdp, start_address, length, dma_mode::vram_fill, vmem_type::vram, fill_data);

	// all setup - wait DMA now
	vdp.wait_dma();

	// assert memory

	// first write should be 2 byte long
	ASSERT_EQ(fill_data, mem.read<std::uint16_t>(start_address));

	// all subsequent written bytes - MSB
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = start_address + ( (i + 1) * 2 );
		ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(addr));
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr + 1)); // DMA shouldn't touch it
	}

	std::uint16_t final_addr = dma_final_address(start_address, length, 2);
	ASSERT_EQ(final_addr, regs.control.address());

	// make sure DMA didn't touch memory after final address
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = final_addr + i;
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr));
	}
}

TEST(VDP_DMA, BASIC_FILL_VRAM_ODD_ADDR_AUTO_INC_1)
{
	test::vdp vdp;
	auto& regs = vdp.registers();
	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& mem = vdp.vram();

	const std::uint16_t start_address = 1;
	const std::uint16_t length = 100;
	const std::uint16_t fill_data = 0xDEAD;
	const std::uint8_t fill_msb = endian::msb(fill_data);
	const std::uint8_t fill_lsb = endian::lsb(fill_data);

	zero_mem(mem);

	// prepare DMA
	regs.R15.INC = 1; // set auto inc
	setup_dma(vdp, start_address, length, dma_mode::vram_fill, vmem_type::vram, fill_data);

	// all setup - wait DMA now
	vdp.wait_dma();

	// assert memory

	// due to byte swap - we should have MSB and LSB
	ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(start_address - 1));
	ASSERT_EQ(fill_lsb, mem.read<std::uint8_t>(start_address));

	// all subsequent written bytes - MSB
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = start_address + 1 + i;
		ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(addr));
	}

	std::uint16_t final_addr = dma_final_address(start_address, length, 1);
	ASSERT_EQ(final_addr, regs.control.address());

	// make sure DMA didn't touch memory after final address
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = final_addr + i;
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr));
	}
}

TEST(VDP_DMA, BASIC_FILL_VRAM_ODD_ADDR_AUTO_INC_2)
{
	test::vdp vdp;
	auto& regs = vdp.registers();
	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& mem = vdp.vram();

	const std::uint16_t start_address = 1;
	const std::uint16_t length = 100;
	const std::uint16_t fill_data = 0xABCD;
	const std::uint8_t fill_msb = endian::msb(fill_data);
	const std::uint8_t fill_lsb = endian::lsb(fill_data);

	zero_mem(mem);
	
	// prepare DMA
	regs.R15.INC = 2; // set auto inc
	setup_dma(vdp, start_address, length, dma_mode::vram_fill, vmem_type::vram, fill_data);

	// all setup - wait DMA now
	vdp.wait_dma();

	// assert memory

	// due to byte swap - we should have MSB and LSB
	ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(start_address - 1));
	ASSERT_EQ(fill_lsb, mem.read<std::uint8_t>(start_address));

	// all subsequent written bytes - MSB
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = start_address + ( (i + 1) * 2 );
		ASSERT_EQ(fill_msb, mem.read<std::uint8_t>(addr));
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr - 1)); // DMA shouldn't touch it
	}

	std::uint16_t final_addr = dma_final_address(start_address, length, 2);
	ASSERT_EQ(final_addr, regs.control.address());

	// make sure DMA didn't touch memory after final address
	for(int i = 0; i < length; ++i)
	{
		std::uint16_t addr = final_addr + i;
		ASSERT_EQ(0, mem.read<std::uint8_t>(addr));
	}
}
