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
	smd(std::string_view rom_path, std::shared_ptr<io_ports::input_device> input_dev1);

	void cycle();

	vdp::vdp& vdp() { return *m_vdp; }

private:
	void build_cpu_memory_map(std::shared_ptr<memory::addressable> rom, genesis::rom& parsed_rom);
	std::shared_ptr<memory::addressable> load_rom(std::string_view rom_path);

	std::uint8_t version_register_value(genesis::rom& parsed_rom) const;

private:
	std::shared_ptr<memory::addressable> m68k_mem_map;
	std::shared_ptr<memory::addressable> z80_mem_map;

	// tmp
	void z80_cycle();
	impl::z80_control_registers m_z80_ctrl_registers;

	std::shared_ptr<memory::memory_unit> z80_request;
	std::shared_ptr<memory::memory_unit> z80_reset;

	bool m_z80_request = false;
	bool m_z80_reset = false;

protected:
	// unique_ptr for simplicity for now
	std::unique_ptr<m68k::cpu> m_m68k_cpu;
	std::unique_ptr<z80::cpu> m_z80_cpu;
	std::unique_ptr<vdp::vdp> m_vdp;

	std::uint32_t prev_pc = 0x0;
	unsigned cycles = 0;

private:
	 std::shared_ptr<io_ports::input_device> m_input_dev1;
};

} // namespace genesis

#endif // __GENESIS_H__
