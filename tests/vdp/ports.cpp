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

void set_register(test::vdp& vdp, std::uint8_t reg_num, std::uint8_t data)
{
	vdp.io_ports().write_control(format_write_register(reg_num, data));
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

TEST(VDP, CONTROL_PORT_ADDRESS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& sett = vdp.sett();
	auto& regs = vdp.registers();

	std::uint8_t reg_num = 10;
	std::uint8_t reg_data = 0;

	for(int data = 0; data <= 0xFFFF; ++data)
	{
		set_register(vdp, reg_num, reg_data);
		ASSERT_EQ(reg_data, regs.get_register(reg_num));

		ports.write_control(format_address_1(data));
		ports.write_control(format_address_2(data));
		ASSERT_EQ(data, sett.control_address());

		// Make sure the pending flag is cleared by setting the register
		++reg_data;
		set_register(vdp, reg_num, reg_data);
		ASSERT_EQ(reg_data, regs.get_register(reg_num));
	}
}

void write_cd_bits(test::vdp& vdp, std::uint8_t cd3_0)
{
	std::uint16_t first = std::uint16_t(cd3_0 & 0b11) << 14;
	std::uint16_t second = std::uint16_t(cd3_0 >> 2) << 4;

	vdp.io_ports().write_control(first);
	vdp.io_ports().write_control(second);
}

TEST(VDP, CONTROL_PORT_VMEM_TYPE)
{
	test::vdp vdp;

	auto& sett = vdp.sett();

	write_cd_bits(vdp, 0b0000);
	ASSERT_EQ(vdp::control_type::read, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::vram, sett.control_mem_type());

	write_cd_bits(vdp, 0b0001);
	ASSERT_EQ(vdp::control_type::write, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::vram, sett.control_mem_type());

	write_cd_bits(vdp, 0b1000);
	ASSERT_EQ(vdp::control_type::read, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::cram, sett.control_mem_type());

	write_cd_bits(vdp, 0b0011);
	ASSERT_EQ(vdp::control_type::write, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::cram, sett.control_mem_type());

	write_cd_bits(vdp, 0b0100);
	ASSERT_EQ(vdp::control_type::read, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::vsram, sett.control_mem_type());

	write_cd_bits(vdp, 0b0101);
	ASSERT_EQ(vdp::control_type::write, sett.control_type());
	ASSERT_EQ(vdp::vmem_type::vsram, sett.control_mem_type());
}

// Start writing 32 bits to control port
void start_long_write(test::vdp& vdp)
{
	auto& ports = vdp.io_ports();

	// make sure pending flag is clear
	ports.read_control();

	// writing 0 will do the trick
	ports.write_control(std::uint16_t(0));
}

TEST(VDP, CONTROL_PORT_WRITE_PENDING)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	std::uint8_t reg_num = 0;
	std::uint8_t reg_data = 0xEF;

	set_register(vdp, reg_num, reg_data);

	// TEST 1: reading from control port must clear the pending flag
	start_long_write(vdp);
	ports.read_control(); // must clear the flag
	set_register(vdp, reg_num, 0);

	// assert 2nd control write is register set, not 2nd word of address
	ASSERT_EQ(0, regs.get_register(0));

	// TEST 2: TODO: reading/writing to data port must clear the pending flag
}
