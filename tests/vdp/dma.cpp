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

TEST(VDP_DMA, FILL_VRAM_BASIC)
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

	// prepare DMA
	regs.R1.M1 = 1; // enable DMA
	regs.R15.INC = 1; // set auto inc to 1
	sett.dma_length(length);
	sett.dma_mode(dma_mode::vram_fill);

	zero_mem(mem);

	control_register control;
	control.address(start_address);
	control.dma_start(true);
	control.vmem_type(vmem_type::vram);
	control.work_completed(false);
	control.control_type(control_type::write); // TODO: does it affect DMA?

	// write control
	ports.init_write_control(control.raw_c1());
	vdp.wait_io_ports();

	ports.init_write_control(control.raw_c2());
	vdp.wait_io_ports();

	// write fill data
	ports.init_write_data(fill_data);
	vdp.wait_io_ports();

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

	// We increment address several times:
	// + 1 (by writing to data port)
	// length - by dma (+ 1 after writing the last byte)
	std::uint16_t final_addr = start_address + (1 * (length + 1));
	ASSERT_EQ(final_addr, regs.control.address());
}
