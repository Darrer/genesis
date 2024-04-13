#ifndef __SMD_H__
#define __SMD_H__

#include "memory/addressable.h"
#include "m68k/cpu.h"
#include "z80/cpu.h"
#include "vdp/vdp.h"
#include "rom.h"

#include "impl/z80_control_registers.h"

#include "io_ports/input_device.h"

#include <memory>
#include <string_view>

namespace genesis
{

// Sega Mega Drive
class smd
{
public:
	smd(const genesis::rom& rom, std::shared_ptr<io_ports::input_device> input_dev1);

	void cycle();

	vdp::vdp& vdp() { return *m_vdp; }

private:
	void build_cpu_memory_map(const genesis::rom& rom);

	static std::unique_ptr<memory::addressable> build_version_register(const genesis::rom& rom);
	static std::shared_ptr<std::vector<std::uint8_t>> load_rom(const genesis::rom& rom);

private:
	std::shared_ptr<memory::addressable> m_m68k_mem_map;
	std::shared_ptr<memory::addressable> m_z80_mem_map;

	// tmp
	void z80_cycle();
	impl::z80_control_registers m_z80_ctrl_registers;

protected:
	std::unique_ptr<m68k::cpu> m_m68k_cpu;
	std::unique_ptr<z80::cpu> m_z80_cpu;
	std::unique_ptr<vdp::vdp> m_vdp;

	unsigned cycles = 0;

private:
	 std::shared_ptr<io_ports::input_device> m_input_dev1;
};

} // namespace genesis

#endif // __GENESIS_H__
