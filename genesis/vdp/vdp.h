#ifndef __VDP_H__
#define __VDP_H__

#include "register_set.h"
#include "settings.h"


namespace genesis::vdp
{

class vdp
{
public:
	vdp() : sett(regs) { }

private:
	register_set regs;
	settings sett;
};

}

#endif // __VDP_H__