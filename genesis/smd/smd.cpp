#include "smd.h"

#include "memory/read_only_memory_unit.h"
#include "memory/memory_builder.h"
#include "memory/memory_unit.h"
#include "memory/dummy_memory.h"

#include "impl/m68k_bus_access.h"
#include "impl/m68k_interrupt_access.h"

#include "io_ports/controller.h"

#include <fstream>


namespace genesis
{

smd::smd(std::string_view rom_path, std::shared_ptr<io_ports::input_device> input_dev1)
	: m_input_dev1(input_dev1)
{
	m_vdp = std::make_unique<vdp::vdp>();

	auto rom = load_rom(rom_path);
	genesis::rom parsed_rom(rom_path);
	build_cpu_memory_map(std::move(rom), parsed_rom);

	// z80::memory z80_mem(z80_mem_map);
	m_z80_cpu = std::make_unique<z80::cpu>(std::make_shared<z80::memory>(z80_mem_map));

	m_m68k_cpu = std::make_unique<m68k::cpu>(m68k_mem_map);

	// TODO: it does not make much senete to have a shared_pointer to an object containing a reference
	auto m68k_bus_access = std::make_shared<impl::m68k_bus_access_impl>(m_m68k_cpu->bus_access());
	m_vdp->set_m68k_bus_access(m68k_bus_access);

	auto m68k_int_access = std::make_shared<impl::m68k_interrupt_access_impl>(*m_m68k_cpu);
	m_vdp->set_m68k_interrupt_access(m68k_int_access);
}

void smd::cycle()
{
	// NOTE: preliminary implementation
	std::uint32_t pc = m_m68k_cpu->registers().PC;
	if(pc != prev_pc)
	{
		prev_pc = pc;
		// std::cout << "PC: " << su::hex_str(pc) << "\n";
	}

	++cycles;

	// TODO: temporary use random numbers
	if(cycles % 8 == 0)
		m_m68k_cpu->cycle();

	if(cycles % 64 == 0)
		z80_cycle();

	m_vdp->cycle();
}

void smd::z80_cycle()
{
	m_z80_ctrl_registers.cycle();

	if(m_z80_ctrl_registers.z80_reset_requested())
	{
		m_z80_cpu->reset();
		return;
	}

	if(m_z80_ctrl_registers.z80_bus_granted())
	{
		return;
	}

	// std::cout << "Run z80\n";
	m_z80_cpu->execute_one();
}

void smd::build_cpu_memory_map(std::shared_ptr<memory::addressable> rom, genesis::rom& parsed_rom)
{
	/* Build z80 memory map */
	memory::memory_builder z80_builder;

	z80_builder.add(std::make_shared<memory::memory_unit>(0x1FFF, std::endian::little), 0x0, 0x1FFF); // main RAM
	z80_builder.add(std::make_shared<memory::dummy_memory>(0x3, std::endian::little), 0x4000, 0x4003);
	z80_builder.add(std::make_shared<memory::dummy_memory>(0x0, std::endian::little), 0x6000, 0x6000);
	z80_builder.add(std::make_shared<memory::memory_unit>(0x1FFB, std::endian::little), 0x4004, 0x5FFF); // sound chip
	z80_builder.add(std::make_shared<memory::zero_memory_unit>(0x0, std::endian::little), 0x7F11, 0x7F11); // bank switch
	z80_builder.add(std::make_shared<memory::zero_memory_unit>(0x7FFF, std::endian::little), 0x8000, 0xFFFF); // ROM

	// z80_builder.add(std::make_shared<memory::memory_unit>(0x1FFF, std::endian::little), 0x2000, 0x3FFF); // reserved
	// z80_builder.add(std::make_shared<memory::memory_unit>(0x1F0F, std::endian::little), 0x6001, 0x7F10); // reserved
	// z80_builder.add(std::make_shared<memory::memory_unit>(0xED, std::endian::little), 0x7F12, 0x7FFF); // reserved

	z80_mem_map = z80_builder.build();

	memory::memory_builder m68k_builder;

	auto m68k_ram = std::make_shared<memory::memory_unit>(0xFFFF, std::endian::big);

	// Setup version register based on loading rom
	auto version_register = std::make_shared<memory::read_only_memory_unit>(0x1, std::endian::big);
	std::uint8_t ver_reg = version_register_value(parsed_rom);
	version_register->write<std::uint16_t>(0, (ver_reg << 8) | ver_reg);


	m68k_builder.add(rom, 0x0, 0x3FFFFF);
	m68k_builder.add(z80_mem_map, 0xA00000, 0xA0FFFF);
	// m68k_builder.add(m68k_ram, 0xFF0000, 0xFFFFFF);
	m68k_builder.add(version_register, 0xA10000, 0xA10001);

	// mirrored every $FFFF
	// TODO: not sure about this behavior
	std::uint32_t start_addr = 0x00E00000;
	for(int i = 0; i < 32; ++i)
	{
		m68k_builder.add(m68k_ram, start_addr, start_addr + 0xFFFF);
		start_addr += 0xFFFF + 1;
	}

	// TMSS register
	m68k_builder.add(std::make_shared<memory::memory_unit>(0x3, std::endian::big), 0xA14000, 0xA14003);

	// TMSS/cartridge register
	m68k_builder.add(std::make_shared<memory::memory_unit>(0x1, std::endian::big), 0xA14101, 0xA14102);

	// Reserved
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x2DFD, std::endian::big), 0xA11202, 0xA13FFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x1EEDFD, std::endian::big), 0xA11202, 0xBFFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0xFFFF, std::endian::big), 0xBF0000, 0xBFFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x3EFFDF, std::endian::big), 0xC00020, 0xFEFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x0, std::endian::big), 0x009FFFFF, 0x009FFFFF);

	// VDP Ports
	m68k_builder.add(m_vdp->io_ports(), 0xC00000, 0xC00007);
	// unemplemented VDP Ports
	m68k_builder.add(std::make_shared<memory::zero_memory_unit>(0x17, std::endian::big), 0xC00008, 0xC0001F);

	/* IO ports */

	// Controller 1
	io_ports::controller controller1(m_input_dev1);
	m68k_builder.add(controller1.data_port(), 0xA10002, 0xA10003);
	m68k_builder.add(controller1.control_port(), 0xA10008, 0xA10009);

	// Controller 2
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x1, std::endian::big), 0xA10004, 0xA10005);
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x1, std::endian::big), 0xA1000A, 0xA1000B);

	// Expansion
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x1, std::endian::big), 0xA10006, 0xA10007);
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x1, std::endian::big), 0xA1000C, 0xA1000D);

	// Serial interface for controllers/expansion
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x11, std::endian::big), 0xA1000E, 0xA1001F);

	/* Z80 control registers */
	m68k_builder.add(m_z80_ctrl_registers.z80_bus_request_register(), 0xA11100, 0xA11101);
	m68k_builder.add(m_z80_ctrl_registers.z80_reset_register(), 0xA11200, 0xA11201);
	
	m68k_mem_map = m68k_builder.build();
}

std::shared_ptr<memory::addressable> smd::load_rom(std::string_view rom_path)
{
	auto rom = std::make_shared<memory::memory_unit>(0x3FFFFF, std::endian::big);

	std::ifstream fs(rom_path.data(), std::ios_base::binary);
	if(!fs.is_open())
		throw std::runtime_error("cannot find rom");

	// TODO: check if ROM is too big
	std::uint32_t offset = 0x0;
	while(fs)
	{
		char c;
		if(fs.get(c))
		{
			rom->write(offset, c);
			++offset;
		}
	}

	return rom;
}

std::uint8_t smd::version_register_value(genesis::rom& parsed_rom) const
{
	auto supports = [&parsed_rom](char region_type)
	{
		return parsed_rom.header().region_support.contains(region_type);
	};

	std::uint8_t reg_value = 0x0;

	if(supports('E') || supports('U'))
	{
		std::cout << "Supports either U/E\n";
		reg_value |= 1 << 7;
	}

	// Use PAL for Europe region and NTSC otherwise
	if(supports('E'))
	{
		reg_value |= 1 << 6; // PAL
	}

	reg_value |= 1 << 5; // Expansion unit is not connected & supported

	reg_value |= 0b0001; // Version number

	return reg_value;
}

} // namespace genesis
