#ifndef __SMD_H__
#define __SMD_H__

#include "memory/addressable.h"
#include "m68k/cpu.h"
#include "vdp/vdp.h"

#include <memory>
#include <string_view>

namespace genesis
{

// Sega Mega Drive
class smd
{
public:
	smd(std::string_view rom_path);

	void cycle();

	vdp::vdp& vdp() { return *m_vdp; }

private:
	void build_cpu_memory_map(std::shared_ptr<memory::addressable> rom);
	std::shared_ptr<memory::addressable> load_rom(std::string_view rom_path);

private:
	std::shared_ptr<memory::addressable> m68k_mem_map;

	// tmp
	std::shared_ptr<memory::memory_unit> z80_request;

protected:
	// unique_ptr for simplicity for now
	std::unique_ptr<m68k::cpu> m_m68k_cpu;
	std::unique_ptr<vdp::vdp> m_vdp;

	// std::shared_ptr<memory::addressable> z80_mem_map;

	std::uint32_t prev_pc = 0x0;
};

} // namespace genesis

#endif // __GENESIS_H__
