#ifndef __SMD_IMPL_M68K_BUS_ACCESS__
#define __SMD_IMPL_M68K_BUS_ACCESS__

#include "vdp/m68k_bus_access.h"
#include "m68k/bus_access.h"

namespace genesis::impl
{

class m68k_bus_access_impl : public genesis::m68k_bus_access
{
public:
	m68k_bus_access_impl(genesis::m68k::bus_access& bus_access) : bus_access(bus_access) { }

	void request_bus() override { bus_access.request_bus(); }
	void release_bus() override { bus_access.release_bus(); }

	void init_read_word(std::uint32_t address) override
	{
		// TODO: assume it's always DATA address space
		bus_access.init_read_word(address, genesis::m68k::addr_space::DATA);
	}

	std::uint16_t latched_word() const override { return bus_access.latched_word(); }

	bool is_idle() const override { return bus_access.is_idle(); }
private:
	genesis::m68k::bus_access& bus_access;
};

}

#endif // __SMD_IMPL_M68K_BUS_ACCESS__
