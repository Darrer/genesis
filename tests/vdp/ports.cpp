#include <gtest/gtest.h>

#include "test_vdp.h"

using namespace genesis;

std::uint16_t format_write_register(std::uint8_t reg, std::uint8_t data)
{
	std::uint16_t res = std::uint16_t(0b100) << 13;
	res |= std::uint16_t(reg) << 8;
	res |= data;
	return res;
}

TEST(VDP, CONTROL_PORT_WRITE_REGISTER)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	for(std::uint8_t reg = 0; reg <= 23; ++ reg)
	{
		for(int data = 0; data <= 255; ++data)
		{
			ports.write_control(format_write_register(reg, data));

			ASSERT_EQ(data, regs.get_register(reg));
		}
	}
}
