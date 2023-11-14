#ifndef __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__
#define __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__

#include "m68k/cpu.h"
#include "vdp/m68k_interrupt_access.h"

namespace genesis::impl
{

class m68k_interrupt_access_impl : public vdp::m68k_interrupt_access
{
public:
	m68k_interrupt_access_impl(genesis::m68k::cpu& cpu) : cpu(cpu) { }
	
	/* vdp::m68k_interrupt_access interface */
	void rise_vertical_interrupt() override
	{
		cpu.set_interrupt(6);
	}

	void rise_horizontal_interrupt() override
	{
		cpu.set_interrupt(4);
	}

	void rise_external_interrupt() override
	{
		cpu.set_interrupt(2);
	}

private:
	genesis::m68k::cpu& cpu;
};

}

#endif // __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__
