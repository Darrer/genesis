#include <gtest/gtest.h>

#include "test_vdp.h"

using namespace genesis;


std::uint32_t wait_ports(test::vdp& vdp)
{
	std::uint32_t cycles = 0;

	auto& ports = vdp.io_ports();
	while (!ports.is_idle())
	{
		vdp.cycle();
		++cycles;
	}

	return cycles;
}

std::uint16_t format_write_register(std::uint8_t reg, std::uint8_t data)
{
	std::uint16_t res = std::uint16_t(0b100) << 13;
	res |= std::uint16_t(reg) << 8;
	res |= data;
	return res;
}

void set_register(test::vdp& vdp, std::uint8_t reg_num, std::uint8_t data)
{
	auto& ports = vdp.io_ports();

	ports.init_write_control(format_write_register(reg_num, data));
	wait_ports(vdp);
}

std::uint16_t format_address_1(std::uint16_t addr)
{
	std::uint16_t res = addr & ~(0b11 << 14);
	return res;
}

std::uint16_t format_address_2(std::uint16_t addr)
{
	std::uint16_t res = addr >> 14;
	return res;
}

TEST(VDP_PORTS, INIT_READ_CONTROL)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	ports.init_read_control();
	wait_ports(vdp);

	ASSERT_TRUE(ports.is_idle());
	ASSERT_EQ(regs.sr_raw, ports.read_result());

	// changing SR should be immediately reflected
	regs.sr_raw = 0x1234;
	ASSERT_EQ(regs.sr_raw, ports.read_result());

	regs.sr_raw = 0x4321;
	ASSERT_EQ(regs.sr_raw, ports.read_result());
}

TEST(VDP_PORTS, WRITE_CONTROL_REGISTERS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	for(std::uint8_t reg = 0; reg <= 23; ++ reg)
	{
		for(int data = 0; data <= 255; ++data)
		{
			ports.init_write_control(format_write_register(reg, data));
			wait_ports(vdp);

			ASSERT_EQ(data, regs.get_register(reg));
		}
	}
}

TEST(VDP_PORTS, WRITE_CONTROL_ADDRESS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& regs = vdp.registers();

	std::uint8_t reg_num = 10;
	std::uint8_t reg_data = 0;

	for(int data = 0; data <= 0xFFFF; ++data)
	{
		std::uint16_t addr1 = format_address_1(data);
		std::uint16_t addr2 = format_address_2(data);

		// write 1st part
		ports.init_write_control(addr1);
		wait_ports(vdp);
		ASSERT_EQ(regs.control.raw_c1(), addr1);

		// write 2nd part
		ports.init_write_control(addr2);
		wait_ports(vdp);
		ASSERT_EQ(regs.control.raw_c2(), addr2);
	}
}

TEST(VDP_PORTS, BYTE_WRITE_CONTROL_REGISTERS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	for(std::uint8_t data = 128; data < 192; ++data)
	{
		std::uint8_t reg_data = data;
		std::uint8_t reg_num = data & 0b11111;

		ports.init_write_control(data);
		wait_ports(vdp);

		// registers should not be affected
		if(reg_num > 23)
			continue;

		ASSERT_TRUE((data >> 6) == 0b10);
		ASSERT_EQ(reg_data, regs.get_register(reg_num));
	}
}

TEST(VDP_PORTS, BYTE_WRITE_CONTROL_ADDRESS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	for(int data = 0; data <= 255; ++data)
	{
		if((data >> 6) == 0b10)
			continue;

		const std::uint16_t expected_data = std::uint16_t(data << 8) | data;

		ports.init_write_control(std::uint8_t(data));
		wait_ports(vdp);
		ASSERT_EQ(expected_data, regs.control.raw_c1());

		ports.init_write_control(std::uint8_t(data));
		wait_ports(vdp);
		ASSERT_EQ(expected_data, regs.control.raw_c2());
	}
}

TEST(VDP_PORTS, CONTROL_PENDING_FLAG)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	const std::uint8_t reg_num = 0;
	const std::uint8_t reg_data = 0xEF;
	const std::uint8_t new_reg_data = 0xFE;

	// TEST 1: after writing 2nd address word pending flag must be cleared
	{
		set_register(vdp, reg_num, reg_data);

		ports.init_write_control(format_address_1(0xDEAD)); // flag must be set
		wait_ports(vdp);

		ports.init_write_control(format_address_2(0xBEEF)); // flag must be cleared
		wait_ports(vdp);

		set_register(vdp, reg_num, new_reg_data);

		ASSERT_EQ(new_reg_data, regs.get_register(reg_num));
	}

	// TEST 2: reading from control port must clear the pending flag
	{
		set_register(vdp, reg_num, reg_data);

		ports.init_write_control(format_address_1(0xDEAD)); // flag must be set
		wait_ports(vdp);

		ports.init_read_control(); // must clear the flag
		wait_ports(vdp);

		set_register(vdp, reg_num, new_reg_data);

		ASSERT_EQ(new_reg_data, regs.get_register(reg_num));
	}

	// TEST 3/4: TODO: reading/writing to data port must clear the pending flag
}

TEST(VDP_PORTS, READ_RESULT_WITH_NO_RESULT)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();

	ASSERT_THROW(ports.read_result(), std::runtime_error);

	ports.init_write_control(std::uint16_t(0));
	ASSERT_THROW(ports.read_result(), std::runtime_error);

	wait_ports(vdp);
	ASSERT_THROW(ports.read_result(), std::runtime_error);
}
