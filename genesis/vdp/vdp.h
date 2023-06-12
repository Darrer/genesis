#ifndef __VDP_H__
#define __VDP_H__

#include <memory>

#include "register_set.h"
#include "settings.h"
#include "ports.h"
#include "memory.h"


namespace genesis::vdp
{

class vdp
{
public:
	vdp();

	// TODO: it should have multiple cycle methods with different clock rate
	void cycle();

	register_set& registers() { return regs; }
	settings& sett() { return _sett; }
	ports& io_ports() { return ports; }

	vram_t& vram() { return *_vram; }
	cram_t& cram() { return _cram; }
	vsram_t vsram() { return _vsram; }

private:
	register_set regs;
	settings _sett;
	ports ports;

	std::unique_ptr<vram_t> _vram;
	cram_t _cram;
	vsram_t _vsram;
};

}

#endif // __VDP_H__