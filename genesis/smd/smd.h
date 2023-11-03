#ifndef __SMD_H__
#define __SMD_H__

#include "memory/addressable.h"
#include "m68k/cpu.h"
#include "vdp/vdp.h"

#include <memory>

namespace genesis
{

// Sega Mega Drive
class smd
{
public:
	smd();

	void cycle();

private:
	void build_cpu_memory_map();

private:
	std::shared_ptr<memory::addressable> m68k_mem_map;

protected:
	// unique_ptr for simplicity for now
	std::unique_ptr<m68k::cpu> m_m68k_cpu;
	std::unique_ptr<vdp::vdp> m_vdp;

	// std::shared_ptr<memory::addressable> z80_mem_map;
};

} // namespace genesis

#endif // __GENESIS_H__
