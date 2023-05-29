#ifndef __VDP_H__
#define __VDP_H__

#include "register_set.h"
#include "settings.h"
#include "ports.h"


namespace genesis::vdp
{

class vdp
{
public:
	vdp() : sett(regs), ports(regs) { }

	register_set& registers() { return regs; }
	// settings& setings() { return sett; }
	ports& io_ports() { return ports; }

private:
	register_set regs;
	settings sett;
	ports ports;
};

}

#endif // __VDP_H__