#include <gtest/gtest.h>

#include "test_vdp.h"
#include "../helpers/random.h"

using namespace genesis;
using namespace genesis::vdp;

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
