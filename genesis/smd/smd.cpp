#include "smd.h"

#include "memory/read_only_memory_unit.h"
#include "memory/memory_builder.h"
#include "memory/memory_unit.h"
#include "memory/dummy_memory.h"
#include "memory/logging_memory.h"

#include "impl/m68k_bus_access.h"
#include "impl/m68k_interrupt_access.h"
#include "impl/z80_io_ports.h"
#include "impl/z80_68bank.h"

#include "io_ports/controller.h"
#include "io_ports/disabled_port.h"

#include <fstream>


namespace genesis
{

smd::smd(genesis::rom rom, std::shared_ptr<io_ports::input_device> input_dev1)
	: m_rom(std::move(rom)), m_input_dev1(input_dev1)
{
	m_vdp = std::make_unique<vdp::vdp>();

	build_cpu_memory_map(load_rom());

	// z80::memory z80_mem(z80_mem_map);
	auto z80_ports = std::make_shared<impl::z80_io_ports>();
	m_z80_cpu = std::make_unique<z80::cpu>(std::make_shared<z80::memory>(z80_mem_map), z80_ports);

	auto m68k_int_access = std::make_shared<impl::m68k_interrupt_access_impl>();
	m_m68k_cpu = std::make_unique<m68k::cpu>(m68k_mem_map, m68k_int_access);

	// TODO: it does not make much senete to have a shared_pointer to an object containing a reference
	auto m68k_bus_access = std::make_shared<impl::m68k_bus_access_impl>(m_m68k_cpu->bus_access());
	m_vdp->set_m68k_bus_access(m68k_bus_access);

	m68k_int_access->set_cpu(*m_m68k_cpu);
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

void smd::build_cpu_memory_map(std::shared_ptr<std::vector<std::uint8_t>> rom_ptr)
{
	/* Build z80 memory map */
	memory::memory_builder z80_builder;

	z80_builder.add_unique(memory::make_memory_unit(0x1FFF, std::endian::little), 0x0, 0x1FFF); // main RAM
	z80_builder.mirror(0x0, 0x1FFF, 0x2000, 0x3FFF); // main RAM mirror

	z80_builder.add_unique(std::make_unique<memory::dummy_memory>(0x0, std::endian::little), 0x4000, 0x4000);
	z80_builder.add_unique(std::make_unique<memory::zero_memory_unit>(0x0, std::endian::little), 0x4001, 0x4001);
	z80_builder.add_unique(std::make_unique<memory::dummy_memory>(0x0, std::endian::little), 0x4002, 0x4002);
	z80_builder.add_unique(std::make_unique<memory::zero_memory_unit>(0x0, std::endian::little), 0x4003, 0x4003);

	// TODO: YM2612 should be repeated from 0x4004 up to 0x5FFF.
	// auto ym2612 = logging(std::make_shared<memory::dummy_memory>(0x3, std::endian::little), "ym2612");
	// auto ym2612 = std::make_shared<memory::dummy_memory>(0x3, std::endian::little);
	// z80_builder.add(ym2612, 0x4000, 0x4003); // YM2612

	// z80_builder.add(std::make_shared<memory::memory_unit>(0x1FFB, std::endian::little), 0x4004, 0x5FFF); // TMP
	z80_builder.add_unique(std::make_unique<memory::dummy_memory>(0x0, std::endian::little), 0x7F11, 0x7F11); // PSG


	// TODO: only rom is accessible for now
	impl::z80_68bank z80_bank(std::make_shared<memory::memory_unit>(rom_ptr, std::endian::big));
	z80_builder.add(z80_bank.bank_register(), 0x6000, 0x6000);
	z80_builder.add(z80_bank.bank_area(), 0x8000, 0xFFFF);

	// z80_builder.add(std::make_shared<memory::memory_unit>(0x1FFF, std::endian::little), 0x2000, 0x3FFF); // reserved
	// z80_builder.add(std::make_shared<memory::memory_unit>(0x1F0F, std::endian::little), 0x6001, 0x7F10); // reserved
	// z80_builder.add(std::make_shared<memory::memory_unit>(0xED, std::endian::little), 0x7F12, 0x7FFF); // reserved

	z80_mem_map = z80_builder.build();

	memory::memory_builder m68k_builder;

	// Setup version register based on the loaded rom
	m68k_builder.add_unique(build_version_register(), 0xA10000, 0xA10001);

	m68k_builder.add_unique(std::make_unique<memory::memory_unit>(rom_ptr, std::endian::big), 0x0, 0x3FFFFF);
	m68k_builder.add(z80_mem_map, 0xA00000, 0xA0FFFF);

	// M68K RAM, mirrored every $FFFF
	const std::uint32_t M68K_RAM_START = 0xE00000;
	const std::uint32_t M68K_RAM_END = 0xE0FFFF;
	const std::uint32_t M68K_RAM_HA = 0xFFFF;

	m68k_builder.add_unique(memory::make_memory_unit(M68K_RAM_HA, std::endian::big), M68K_RAM_START, M68K_RAM_END);
	for(int i = 1; i <= 32; ++i)
	{
		std::uint32_t mirror_start = M68K_RAM_START + ((M68K_RAM_HA + 1) * i);
		std::uint32_t mirror_end = mirror_start + M68K_RAM_HA;
		m68k_builder.mirror(M68K_RAM_START, M68K_RAM_END, mirror_start, mirror_end);
	}

	// TMSS register
	m68k_builder.add_unique(memory::make_memory_unit(0x3, std::endian::big), 0xA14000, 0xA14003);

	// TMSS/cartridge register
	m68k_builder.add_unique(memory::make_memory_unit(0x1, std::endian::big), 0xA14101, 0xA14102);

	// Reserved
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x2DFD, std::endian::big), 0xA11202, 0xA13FFF);
	m68k_builder.add_unique(memory::make_memory_unit(0x1EEDFD, std::endian::big), 0xA11202, 0xBFFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0xFFFF, std::endian::big), 0xBF0000, 0xBFFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x3EFFDF, std::endian::big), 0xC00020, 0xFEFFFF);
	// m68k_builder.add(std::make_shared<memory::memory_unit>(0x0, std::endian::big), 0x009FFFFF, 0x009FFFFF);

	// VDP Ports
	m68k_builder.add(m_vdp->io_ports(), 0xC00000, 0xC00007);
	m68k_builder.add_unique(std::make_unique<memory::dummy_memory>(0x17, std::endian::big), 0xC00008, 0xC0001F);

	/* IO ports */

	// Controller 1
	io_ports::controller controller1(m_input_dev1);
	m68k_builder.add(controller1.data_port(), 0xA10002, 0xA10003);
	m68k_builder.add(controller1.control_port(), 0xA10008, 0xA10009);

	// Controller 2
	m68k_builder.add_unique(io_ports::disabled_port::data(), 0xA10004, 0xA10005);
	m68k_builder.add_unique(io_ports::disabled_port::control(), 0xA1000A, 0xA1000B);

	// Expansion
	m68k_builder.add_unique(io_ports::disabled_port::data(), 0xA10006, 0xA10007);
	m68k_builder.add_unique(io_ports::disabled_port::control(), 0xA1000C, 0xA1000D);

	// Serial interface for controllers/expansion
	m68k_builder.add_unique(std::make_unique<memory::zero_memory_unit>(0x11, std::endian::big), 0xA1000E, 0xA1001F);

	/* Z80 control registers */
	m68k_builder.add(m_z80_ctrl_registers.z80_bus_request_register(), 0xA11100, 0xA11101);
	m68k_builder.add(m_z80_ctrl_registers.z80_reset_register(), 0xA11200, 0xA11201);
	
	m68k_mem_map = m68k_builder.build();
}

std::unique_ptr<memory::addressable> smd::build_version_register() const
{
	auto supports = [this](char region_type)
	{
		return m_rom.header().region_support.contains(region_type);
	};

	std::uint8_t reg_value = 0x0;

	if(supports('E') || supports('U'))
	{
		reg_value |= 1 << 7;
	}

	// Use PAL for Europe region and NTSC otherwise
	if(supports('E'))
	{
		reg_value |= 1 << 6; // PAL
	}

	reg_value |= 1 << 5; // The expansion unit is neither connected nor supported.

	reg_value |= 0b0001; // Version number

	auto version_register = std::make_unique<memory::read_only_memory_unit>(0x1, std::endian::big);
	version_register->write<std::uint16_t>(0, reg_value);
	return version_register;
}

std::shared_ptr<std::vector<std::uint8_t>> smd::load_rom()
{
	std::vector<std::uint8_t> data;
	data.assign_range(m_rom.data());
	for(std::size_t i = data.size(); i < 0x400000; ++i)
		data.push_back(0);
	return std::make_shared<std::vector<std::uint8_t>>(std::move(data));
}

} // namespace genesis
