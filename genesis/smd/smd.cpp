#include "smd.h"

#include "memory/memory_builder.h"
#include "memory/memory_unit.h"

#include "impl/m68k_bus_access.h"


namespace genesis
{

smd::smd()
{
	build_cpu_memory_map();

	m_m68k_cpu = std::make_unique<m68k::cpu>(m68k_mem_map);

	// TODO: it does not make much senete to have a shared_pointer to an object containing a reference
	auto m68k_bus_access = std::make_shared<impl::m68k_bus_access_impl>(m_m68k_cpu->bus_access());
	m_vdp = std::make_unique<vdp::vdp>(std::move(m68k_bus_access));
}

void smd::cycle()
{
	// NOTE: preliminary implementation
	m_m68k_cpu->cycle();
	m_vdp->cycle();
}

void smd::build_cpu_memory_map()
{
	auto m68k_ram = std::make_shared<memory::memory_unit>(0xFFFF);
	auto z80_ram = std::make_shared<memory::memory_unit>(0xFFFF);
	auto rom = std::make_shared<memory::memory_unit>(0x3FFFFF);

	memory::memory_builder m68k_builder;
	m68k_builder.add(rom, 0x0, 0x3FFFFF);
	m68k_builder.add(z80_ram, 0xA00000, 0xA0FFFF);
	m68k_builder.add(m68k_ram, 0xFF0000, 0xFFFFFF);

	m68k_mem_map = m68k_builder.build();
}

} // namespace genesis
