#include "smd.h"

#include "memory/read_only_memory_unit.h"
#include "memory/memory_builder.h"
#include "memory/memory_unit.h"
#include "memory/dummy_memory.h"

#include "rom.h"

#include "impl/m68k_bus_access.h"

#include <fstream>


namespace genesis
{

smd::smd(std::string_view rom_path)
{
	m_vdp = std::make_unique<vdp::vdp>();

	auto rom = load_rom(rom_path);
	build_cpu_memory_map(std::move(rom));

	m_m68k_cpu = std::make_unique<m68k::cpu>(m68k_mem_map);

	// TODO: it does not make much senete to have a shared_pointer to an object containing a reference
	auto m68k_bus_access = std::make_shared<impl::m68k_bus_access_impl>(m_m68k_cpu->bus_access());
	m_vdp->set_m68k_bus_access(m68k_bus_access);
}

void smd::cycle()
{
	// NOTE: preliminary implementation
	std::uint32_t pc = m_m68k_cpu->registers().PC;
	if(pc != prev_pc)
	{
		prev_pc = pc;
		std::cout << "PC: " << su::hex_str(pc) << "\n";
	}

	m_m68k_cpu->cycle();
	m_vdp->cycle();
}

void smd::build_cpu_memory_map(std::shared_ptr<memory::addressable> rom)
{
	auto m68k_ram = std::make_shared<memory::memory_unit>(0xFFFF, std::endian::big);
	auto z80_ram = std::make_shared<memory::memory_unit>(0xFFFF, std::endian::little);
	auto version_register = std::make_shared<memory::read_only_memory_unit>(0x1, std::endian::big);
	version_register->write<std::uint16_t>(0, 0);


	memory::memory_builder m68k_builder;
	m68k_builder.add(rom, 0x0, 0x3FFFFF);
	m68k_builder.add(z80_ram, 0xA00000, 0xA0FFFF);
	m68k_builder.add(m68k_ram, 0xFF0000, 0xFFFFFF);
	m68k_builder.add(version_register, 0xA10000, 0xA10001);
	m68k_builder.add(m_vdp->io_ports(), 0xC00000, 0xC0001F);

	// TODO: I don't know what it is
	m68k_builder.add(std::make_shared<memory::memory_unit>(0x1, std::endian::big), 0xA14101, 0xA14102);

	// TODO: Controller 1 data/port
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA10002, 0xA10003);
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA10008, 0xA10009);

	// TODO: Controller 2 data/port
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA10004, 0xA10005);
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA1000A, 0xA1000B);

	// Misc
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x13, std::endian::big), 0xA1000C, 0xA1001F);

	/* Z80 bus */
	
	// Z80 bus request
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA11100, 0xA11101);
	// Z80 reset
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA11200, 0xA11201);

	m68k_mem_map = m68k_builder.build();
}

std::shared_ptr<memory::addressable> smd::load_rom(std::string_view rom_path)
{
	auto rom = std::make_shared<memory::read_only_memory_unit>(0x3FFFFF, std::endian::big);

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

} // namespace genesis
