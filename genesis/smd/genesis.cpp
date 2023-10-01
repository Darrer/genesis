#include "genesis.h"

#include "memory/memory_builder.h"
#include "memory/memory_unit.h"


namespace genesis
{

void genesis::build_cpu_memory_map()
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
