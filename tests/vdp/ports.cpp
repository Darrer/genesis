#include "../helpers/random.h"
#include "test_vdp.h"
#include "vdp/impl/color.h"

#include <gtest/gtest.h>
#include <iostream>

using namespace genesis::vdp;
using namespace genesis;


std::uint32_t wait_ports(test::vdp& vdp, std::string desc = "")
{
	std::uint32_t cycles = 0;
	const std::uint32_t wait_limit = 100'000;

	auto& ports = vdp.io_ports();
	while(!ports.is_idle())
	{
		vdp.cycle();
		++cycles;

		if(cycles >= wait_limit)
			throw std::runtime_error(std::string("wait_ports: it takes too long") + " " + desc);
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

control_register setup_control(std::uint16_t addr, vmem_type mem_type, control_type ctype)
{
	control_register control;
	control.address(addr);
	control.vmem_type(mem_type);
	control.control_type(ctype);
	control.dma_start(false);
	control.work_completed(false);
	return control;
}

control_register setup_control_read(std::uint16_t addr, vmem_type mem_type = vmem_type::vram)
{
	control_register control;
	control.address(addr);
	control.vmem_type(mem_type);
	control.control_type(control_type::read);
	control.dma_start(false);
	control.work_completed(false);

	return control;
}

void setup_control_register(test::vdp& vdp, std::uint16_t addr, vmem_type mem_type = vmem_type::vram)
{
	auto& regs = vdp.registers();
	regs.control = setup_control_read(addr, mem_type);
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

	for(std::uint8_t reg = 0; reg <= 23; ++reg)
	{
		for(int data = 0; data <= 255; ++data)
		{
			ports.init_write_control(format_write_register(reg, data));
			wait_ports(vdp);

			ASSERT_EQ(data, regs.get_register(reg));
		}
	}
}

const std::uint16_t cd5_clear_mask = 0b1111111101111111;

TEST(VDP_PORTS, WRITE_CONTROL_ADDRESS)
{
	test::vdp vdp;

	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();

	for(int data = 0; data <= 0xFFFF; ++data)
	{
		std::uint16_t addr1 = format_address_1(data);
		std::uint16_t addr2 = format_address_2(data) & cd5_clear_mask;

		ASSERT_EQ(0, regs.R1.M1);

		// write 1st part
		ports.init_write_control(addr1);
		vdp.wait_io_ports();
		ASSERT_EQ(addr1, regs.control.raw_c1());

		// write 2nd part
		ports.init_write_control(addr2);
		vdp.wait_io_ports();
		ASSERT_EQ(addr2, regs.control.raw_c2());
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
		vdp.wait_io_ports();
		ASSERT_EQ(expected_data, regs.control.raw_c1());

		ports.init_write_control(std::uint8_t(data));
		vdp.wait_io_ports();
		ASSERT_EQ(expected_data & cd5_clear_mask, regs.control.raw_c2());
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

	// TEST 3: reading from data port must clear the pending flag
	{
		set_register(vdp, reg_num, reg_data);

		// setup control register so we can read it letter
		setup_control_register(vdp, 0xDEAD);

		ports.init_write_control(format_address_1(0xDEAD)); // flag must be set
		wait_ports(vdp);

		ports.init_read_data(); // must clear the flag
		wait_ports(vdp);

		set_register(vdp, reg_num, new_reg_data);

		ASSERT_EQ(new_reg_data, regs.get_register(reg_num));
	}

	// TEST 4: writing to data port must clear the pending flag
	{
		set_register(vdp, reg_num, reg_data);

		// setup control register so we can write to it letter
		regs.control = setup_control(0, vmem_type::vram, control_type::write);

		ports.init_write_control(format_address_1(0xDEAD)); // flag must be set
		wait_ports(vdp);

		ports.init_write_data(std::uint16_t(0)); // must clear the flag
		wait_ports(vdp);

		set_register(vdp, reg_num, new_reg_data);
		ASSERT_EQ(new_reg_data, regs.get_register(reg_num));
	}
}

TEST(VDP_PORTS, READ_RESULT_BAD_ACCESS)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();

	ASSERT_THROW(ports.read_result(), std::runtime_error);

	ports.init_write_control(std::uint16_t(0));
	ASSERT_THROW(ports.read_result(), std::runtime_error);

	wait_ports(vdp);
	ASSERT_THROW(ports.read_result(), std::runtime_error);
}

TEST(VDP_PORTS, DATA_PORT_READ_VRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();
	auto& mem = vdp.vram();

	control_register control = setup_control(0, vmem_type::vram, control_type::read);

	for(int addr = 0; addr <= 0xFFFF - 2; ++addr)
	{
		std::uint16_t expected_data = test::random::next<std::uint16_t>();
		int effective_address = addr & ~1;

		// prepare mem
		mem.write(effective_address, expected_data);

		// setup control register
		control.address(addr);
		regs.control = control;

		// setup read
		ports.init_read_data();
		wait_ports(vdp);

		ASSERT_EQ(expected_data, ports.read_result());
	}
}

TEST(VDP_PORTS, DATA_PORT_READ_CRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.cram();

	control_register control;
	control.vmem_type(vmem_type::cram);
	control.control_type(control_type::read);
	control.dma_start(false);
	control.work_completed(false);

	for(int red = 0; red <= 7; ++red)
	{
		for(int green = 0; green <= 7; ++green)
		{
			for(int blue = 0; blue <= 7; ++blue)
			{
				impl::color color;
				color.red = red;
				color.green = green;
				color.blue = blue;

				std::uint16_t expected_color = 0;
				expected_color |= red << 1;
				expected_color |= green << 5;
				expected_color |= blue << 9;

				ASSERT_EQ(expected_color, color.value());

				for(int addr = 0; addr <= 127; ++addr)
				{
					// prepare mem
					const std::uint16_t expected_data = color.value();
					mem.write(addr, expected_data);
					ASSERT_EQ(expected_data, mem.read(addr));

					control.address(addr);

					// setup control register
					ports.init_write_control(control.raw_c1());
					wait_ports(vdp, "w1");

					ports.init_write_control(control.raw_c2());
					wait_ports(vdp, "w2");

					// setup read
					// ASSERT_EQ(false, vdp.registers().control.dma_start());
					ASSERT_EQ(0, vdp.registers().R1.M1);
					ports.init_read_data();
					wait_ports(vdp, "r1");

					ASSERT_EQ(expected_data, ports.read_result());
				}
			}
		}
	}
}

TEST(VDP_PORTS, DATA_PORT_READ_VSRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.vsram();

	control_register control;
	control.vmem_type(vmem_type::vsram);
	control.control_type(control_type::read);
	control.dma_start(false);
	control.work_completed(false);

	for(int data = 0; data <= 1024; ++data)
	{
		for(int addr = 0; addr <= 79; ++addr)
		{
			// prepare mem
			mem.write(addr, data);
			ASSERT_EQ(data, mem.read(addr));

			control.address(addr);

			// setup control register
			ports.init_write_control(control.raw_c1());
			wait_ports(vdp);

			ports.init_write_control(control.raw_c2());
			wait_ports(vdp);

			// setup read
			ports.init_read_data();
			wait_ports(vdp);

			ASSERT_EQ(data, ports.read_result());
		}
	}
}

TEST(VDP_PORTS, DATA_PORT_WRITE_VRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.vram();
	auto& regs = vdp.registers();

	control_register control = setup_control(0, vmem_type::vram, control_type::write);

	for(int addr = 0; addr <= 0xFFFF - 2; ++addr)
	{
		int effective_address = addr & ~1;
		std::uint16_t data_to_write = test::random::next<std::uint16_t>();
		std::uint16_t expected_data = data_to_write;
		if(addr % 2 == 1)
			endian::swap(expected_data);

		// prepare mem
		mem.write<std::uint8_t>(effective_address, 0);
		mem.write<std::uint8_t>(effective_address + 1, 0);

		control.address(addr);

		// setup control register
		regs.control = control;

		// setup write
		ports.init_write_data(data_to_write);
		wait_ports(vdp);

		vdp.wait_fifo();

		ASSERT_EQ(expected_data, mem.read<std::uint16_t>(effective_address));
	}
}

TEST(VDP_PORTS, DATA_PORT_WRITE_CRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.cram();
	auto& regs = vdp.registers();

	control_register control;
	control.vmem_type(vmem_type::cram);
	control.control_type(control_type::write);
	control.dma_start(false);
	control.work_completed(false);

	std::uint16_t data_to_write = 0xBEEF;
	for(int addr = 0; addr <= 127; ++addr)
	{
		// prepare mem
		mem.write(addr, 0);

		control.address(addr);

		// setup control register
		regs.control = control;

		// setup write
		ports.init_write_data(data_to_write);
		wait_ports(vdp);

		vdp.wait_fifo();

		ASSERT_EQ(data_to_write, mem.read(addr));
	}
}

TEST(VDP_PORTS, DATA_PORT_WRITE_VSRAM)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.vsram();
	auto& regs = vdp.registers();

	control_register control;
	control.vmem_type(vmem_type::vsram);
	control.control_type(control_type::write);
	control.dma_start(false);
	control.work_completed(false);

	std::uint16_t data_to_write = 0xBEEF;
	for(int addr = 0; addr <= 79; ++addr)
	{
		// prepare mem
		mem.write(addr, 0);

		control.address(addr);

		// setup control register
		regs.control = control;

		// setup write
		ports.init_write_data(data_to_write);
		wait_ports(vdp);

		vdp.wait_fifo();

		ASSERT_EQ(data_to_write, mem.read(addr));
	}
}

TEST(VDP_PORTS, DATA_PORT_CRAM_ADDRESS_WRAP)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.cram();
	auto& regs = vdp.registers();

	control_register write_ctrl = setup_control(0, vmem_type::cram, control_type::write);
	control_register read_ctrl = setup_control(0, vmem_type::cram, control_type::read);

	impl::color color;
	for(int addr = 0; addr <= 0xFFFF - 1; ++addr)
	{
		const int effective_addr = addr & 0b0000000001111110;
		std::uint16_t data = test::random::next<std::uint16_t>();
		color.value(data);

		// setup write
		write_ctrl.address(addr);
		regs.control = write_ctrl;

		ports.init_write_data(data);
		vdp.wait_io_ports();

		// we can be sure only for some bits
		ASSERT_TRUE((mem.read(effective_addr) & color.value()) == color.value());

		// setup read
		read_ctrl.address(addr);
		regs.control = read_ctrl;

		ports.init_read_data();
		vdp.wait_io_ports();

		// we can be sure only for some bits
		ASSERT_TRUE((ports.read_result() & color.value()) == color.value());
	}
}

TEST(VDP_PORTS, DATA_PORT_VSRAM_ADDRESS_WRAP)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& mem = vdp.vsram();
	auto& regs = vdp.registers();

	control_register write_ctrl = setup_control(0, vmem_type::vsram, control_type::write);
	control_register read_ctrl = setup_control(0, vmem_type::vsram, control_type::read);

	for(int addr = 0; addr <= 0xFFFF - 1; ++addr)
	{
		const int effective_addr = addr & 0b0000000001111110;
		const std::uint16_t data = test::random::next<std::uint16_t>();
		const std::uint16_t expected_data = data & 0b0000001111111111;

		// setup write
		write_ctrl.address(addr);
		regs.control = write_ctrl;

		ports.init_write_data(data);
		vdp.wait_io_ports();


		if(effective_addr < 80)
		{
			// we can be sure only for some bits
			ASSERT_TRUE((mem.read(effective_addr) & expected_data) == expected_data);
		}
		else
		{
			// TODO: write should be ignored
		}

		// setup read
		read_ctrl.address(addr);
		regs.control = read_ctrl;

		ports.init_read_data();
		vdp.wait_io_ports();

		if(effective_addr < 80)
		{
			// we can be sure only for some bits
			ASSERT_TRUE((ports.read_result() & expected_data) == expected_data);
		}
		else
		{
			// TODO: so far it's not clear how handle addresses >=80
		}
	}
}

TEST(VDP_PORTS, DATA_PORT_READ_VRAM_AUTO_INC)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();
	auto& mem = vdp.vram();

	// prepare mem
	for(std::size_t addr = 0; addr <= mem.max_address(); ++addr)
		mem.write(static_cast<std::uint16_t>(addr), test::random::next<std::uint8_t>());

	control_register control = setup_control(0, vmem_type::vram, control_type::read);

	std::initializer_list<std::uint8_t> auto_inc_values = {
		1, 2, 3, 4, test::random::next<std::uint8_t>(), test::random::next<std::uint8_t>()};

	for(auto auto_inc : auto_inc_values)
	{
		regs.R15.INC = auto_inc;

		std::uint16_t expected_address = 0;
		control.address(expected_address);
		regs.control = control;

		const int num_tests = 10;
		for(auto i = 0; i < num_tests; ++i)
		{
			int effective_address = expected_address & ~1;
			std::uint16_t expected_data = mem.read<std::uint16_t>(effective_address);

			// setup read
			ports.init_read_data();
			vdp.wait_io_ports();

			ASSERT_EQ(expected_data, ports.read_result());

			expected_address += auto_inc;
		}
	}
}

TEST(VDP_PORTS, DATA_PORT_WRITE_VRAM_AUTO_INC)
{
	test::vdp vdp;
	auto& ports = vdp.io_ports();
	auto& regs = vdp.registers();
	auto& mem = vdp.vram();

	control_register control = setup_control(0, vmem_type::vram, control_type::write);

	std::initializer_list<std::uint8_t> auto_inc_values = {
		1, 2, 3, 4, test::random::next<std::uint8_t>(), test::random::next<std::uint8_t>()};

	for(auto auto_inc : auto_inc_values)
	{
		regs.R15.INC = auto_inc;

		std::uint16_t expected_address = 0;
		control.address(expected_address);
		regs.control = control;

		const int num_tests = 10;
		for(auto i = 0; i < num_tests; ++i)
		{
			int effective_address = expected_address & ~1;
			std::uint16_t data = test::random::next<std::uint16_t>();

			// setup write
			ports.init_write_data(data);
			vdp.wait_write();

			std::uint16_t expected_data = data;
			if(expected_address % 2 == 1)
				endian::swap(expected_data);

			ASSERT_EQ(expected_data, mem.read<std::uint16_t>(effective_address));

			expected_address += auto_inc;
		}
	}
}
