#ifndef __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__
#define __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__

#include "m68k/cpu.h"
#include "m68k/interrupting_device.h"
#include "vdp/m68k_interrupt_access.h"

#include <optional>
#include <functional>

namespace genesis::impl
{

class m68k_interrupt_access_impl : public m68k::interrupting_device, public vdp::m68k_interrupt_access
{
public:
	m68k_interrupt_access_impl() = default;

	void set_cpu(genesis::m68k::cpu& cpu)
	{
		m_cpu = cpu;
	}

	/* m68k::interrupting_device interface */
	bool is_idle() const override
	{
		return true;
	}

	void init_interrupt_ack(m68k::cpu_bus& bus, std::uint8_t priority) override
	{
		m_priority = priority;

		if(m_int_cb)
			m_int_cb(priority);
	}

	m68k::interrupt_type interrupt_type() const override
	{
		return m68k::interrupt_type::autovectored;
	}

	std::uint8_t vector_number() const override
	{
		return autovectored(m_priority);
	}

	/* vdp::m68k_interrupt_access interface */

	void interrupt_priority(std::uint8_t ipl) override
	{
		m_cpu.value().get().set_interrupt(ipl);
	}

	std::uint8_t interrupt_priority() const override
	{
		return m_cpu.value().get().bus().interrupt_priority();
	}

	void set_interrupt_callback(interrupt_callback cb) override
	{
		m_int_cb = cb;
	}

private:
	std::uint8_t m_priority = 0;
	vdp::m68k_interrupt_access::interrupt_callback m_int_cb;
	std::optional<std::reference_wrapper<genesis::m68k::cpu>> m_cpu;
};

}

#endif // __SMD_IMPL_M68K_INTERRUPT_ACCESS_H__
