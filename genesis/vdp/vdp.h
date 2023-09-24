#ifndef __VDP_H__
#define __VDP_H__

#include <memory>

#include "register_set.h"
#include "settings.h"
#include "ports.h"
#include "memory.h"
#include "m68k_bus_access.h"

#include "impl/dma.h"


namespace genesis::vdp
{

class vdp
{
public:
	vdp(std::shared_ptr<genesis::m68k_bus_access> m68k_bus);

	// TODO: it should have multiple cycle methods with different clock rate
	void cycle();

	register_set& registers() { return regs; }
	settings& sett() { return _sett; }
	genesis::vdp::ports& io_ports() { return ports; }

	vram_t& vram() { return *_vram; }
	cram_t& cram() { return _cram; }
	vsram_t& vsram() { return _vsram; }

private:
	void handle_ports_requests();
	void handle_dma_requests();

	bool pre_cache_read_is_required() const;

	// TODO: refactor this interface
	void vram_write(std::uint32_t address, std::uint8_t data);
	void cram_write(std::uint32_t address, std::uint16_t data);
	void vsram_write(std::uint32_t address, std::uint16_t data);

private:
	register_set regs;
	settings _sett;
	genesis::vdp::ports ports;

	std::unique_ptr<vram_t> _vram;
	cram_t _cram;
	vsram_t _vsram;

protected:
	impl::memory_access dma_memory;
	impl::dma dma;
};

}

#endif // __VDP_H__