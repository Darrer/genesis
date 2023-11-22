#include "smd.h"

#include "memory/read_only_memory_unit.h"
#include "memory/memory_builder.h"
#include "memory/memory_unit.h"
#include "memory/dummy_memory.h"

#include "rom.h"

#include "impl/m68k_bus_access.h"
#include "impl/m68k_interrupt_access.h"

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

	// TMP: z80 assert bus is granted
	z80_request->write(0, std::uint16_t(0));

	m_m68k_cpu->cycle();

	for(int i = 0; i < 8; ++i)
		m_vdp->cycle();
}

void smd::build_cpu_memory_map(std::shared_ptr<memory::addressable> rom)
{
	auto m68k_ram = std::make_shared<memory::memory_unit>(0xFFFF, std::endian::big);
	auto z80_ram = std::make_shared<memory::dummy_memory>(0xFFFF, std::endian::little);
	auto version_register = std::make_shared<memory::read_only_memory_unit>(0x1, std::endian::big);
	version_register->write<std::uint16_t>(0, 0xFFFF);
	// version_register->write<std::uint16_t>(0, 0b10000001);
	// version_register->write<std::uint16_t>(0, 0b0001111100011111);


	memory::memory_builder m68k_builder;
	m68k_builder.add(rom, 0x0, 0x3FFFFF);
	m68k_builder.add(z80_ram, 0xA00000, 0xA0FFFF);
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
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x17, std::endian::big), 0xC00008, 0xC0001F);

	// Controller 1/2 and Expansion port
	m68k_builder.add(std::make_shared<memory::ffff_memory_unit>(0x1D, std::endian::big), 0xA10002, 0xA1001F);

	/* Z80 bus */
	
	// Z80 bus request

	z80_request = std::make_shared<memory::memory_unit>(0x1, std::endian::big);
	m68k_builder.add(z80_request, 0xA11100, 0xA11101);
	// Z80 reset
	m68k_builder.add(std::make_shared<memory::dummy_memory>(0x1, std::endian::big), 0xA11200, 0xA11201);

	m68k_mem_map = m68k_builder.build();
}

std::shared_ptr<memory::addressable> smd::load_rom(std::string_view rom_path)
{
	// should be read_only_memory_unit
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
