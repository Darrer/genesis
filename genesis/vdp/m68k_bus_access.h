#ifndef __VDP_M68K_BUS_ACCESS_H__
#define __VDP_M68K_BUS_ACCESS_H__

#include <cstdint>

namespace genesis::vdp
{

class m68k_bus_access
{
public:
	virtual ~m68k_bus_access() { }

	virtual void request_bus() = 0;
	virtual void release_bus() = 0;

	virtual void init_read_word(std::uint32_t address) = 0;
	virtual std::uint16_t latched_word() const = 0;

	virtual bool is_idle() const = 0;
};

};

#endif // __VDP_M68K_BUS_ACCESS_H__
