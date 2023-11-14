#ifndef __VDP_M68K_INTERRUPT_ACCESS_H__
#define __VDP_M68K_INTERRUPT_ACCESS_H__

namespace genesis::vdp
{

class m68k_interrupt_access
{
public:
	virtual ~m68k_interrupt_access() = default;

	virtual void rise_vertical_interrupt() = 0;
	virtual void rise_horizontal_interrupt() = 0;
	virtual void rise_external_interrupt() = 0;
};

}

#endif // __VDP_M68K_INTERRUPT_ACCESS_H__
