#ifndef __VDP_H__
#define __VDP_H__

#include "impl/dma.h"
#include "impl/render.h"
#include "m68k_bus_access.h"
#include "m68k_interrupt_access.h"
#include "memory.h"
#include "ports.h"
#include "register_set.h"
#include "settings.h"

#include <memory>
#include <functional>


namespace genesis::vdp
{

class vdp
{
public:
	vdp(std::shared_ptr<m68k_bus_access> m68k_bus);

	vdp() : vdp(nullptr) { }

	void set_m68k_bus_access(std::shared_ptr<m68k_bus_access> m68k_bus)
	{
		dma.set_m68k_bus_access(m68k_bus);
	}

	void set_m68k_interrupt_access(std::shared_ptr<m68k_interrupt_access> m68k_int)
	{
		this->m68k_int = m68k_int;
	}

	// TODO: it should have multiple cycle methods with different clock rate
	void cycle();

	register_set& registers()
	{
		return regs;
	}

	settings& sett()
	{
		return _sett;
	}

	genesis::vdp::ports& io_ports()
	{
		return ports;
	}

	vram_t& vram()
	{
		return _vram;
	}

	cram_t& cram()
	{
		return _cram;
	}

	vsram_t& vsram()
	{
		return _vsram;
	}

	impl::render& render() { return m_render; }

	void on_frame_end(std::function<void()> callback)
	{
		on_frame_end_callback = callback;
	}

private:
	void handle_ports_requests();
	void handle_dma_requests();

	void on_start_scanline();
	void on_end_scanline();
	void check_interrupts();

	bool pre_cache_read_is_required() const;

	void update_status_register();

	// TODO: refactor this interface
	void vram_write(std::uint32_t address, std::uint8_t data);
	void cram_write(std::uint32_t address, std::uint16_t data);
	void vsram_write(std::uint32_t address, std::uint16_t data);

private:
	register_set regs;
	settings _sett;
	genesis::vdp::ports ports;

	vram_t _vram;
	cram_t _cram;
	vsram_t _vsram;

	bool vcounter_flag = false;
	std::uint8_t hint_counter = 0;
	unsigned cycles = 0;

protected:
	impl::memory_access dma_memory;
	impl::dma dma;
	impl::render m_render;
	std::shared_ptr<m68k_interrupt_access> m68k_int;

private:
	std::function<void()> on_frame_end_callback;
};

} // namespace genesis::vdp

#endif // __VDP_H__