#ifndef __VDP_H__
#define __VDP_H__

#include "impl/dma.h"
#include "impl/render.h"
#include "impl/hv_counters.h"
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
		this->m68k_int->set_interrupt_callback([this](std::uint8_t ipl) { on_interrupt(ipl); } );
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

	// must be called before VINT/HINT
	void on_frame_end(std::function<void()> callback)
	{
		on_frame_end_callback = callback;
	}

private:
	void handle_ports_requests();
	void handle_dma_requests();

	void update_status_register();

	void on_start_scanline();
	void on_end_scanline();
	void on_scanline();

	void check_interrupts();
	void on_interrupt(std::uint8_t ipl);

	void update_hv_counters();
	void inc_h_counter();
	void inc_v_counter();

	void update_vblank(display_height height, bool pal);
	void update_hblank(display_width width);

	bool pre_cache_read_is_required() const;

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

	impl::h_counter m_h_counter;
	impl::v_counter m_v_counter;

	std::uint8_t hint_counter = 0;
	unsigned mclk = 0;

protected:
	impl::memory_access dma_memory;
	impl::dma dma;
	impl::render m_render;
	std::shared_ptr<m68k_interrupt_access> m68k_int;

	bool m_vint_pending = false;
	bool m_hint_pending = false;

	int m_scanline = 0;

private:
	std::function<void()> on_frame_end_callback;
};

} // namespace genesis::vdp

#endif // __VDP_H__