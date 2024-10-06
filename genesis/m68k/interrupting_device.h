#ifndef __M68K_INTERRUPTING_DEVICE_H__
#define __M68K_INTERRUPTING_DEVICE_H__

#include "m68k/cpu_bus.hpp"

#include <cstdint>
#include <stdexcept>

namespace genesis::m68k
{

enum class interrupt_type
{
	vectored,
	autovectored,
	spurious
};

const std::uint8_t uninitialized_vector_number = 15;
const std::uint8_t spurious_vector_number = 24;

class interrupting_device
{
public:
	virtual ~interrupting_device() = default;

	virtual bool is_idle() const = 0;
	virtual void init_interrupt_ack(m68k::cpu_bus& bus, std::uint8_t priority) = 0;

	virtual m68k::interrupt_type interrupt_type() const = 0;
	virtual std::uint8_t vector_number() const = 0;

protected:
	// helper method
	static std::uint8_t autovectored(std::uint8_t priority)
	{
		if(priority > 7)
			throw std::invalid_argument("interrupting_device::autovectored: priority");

		return 0x18 + priority;
	}
};

// always generates autovectored interrupts
class autovectored_interrupting_device : public interrupting_device
{
public:
	bool is_idle() const override
	{
		return true;
	}

	void init_interrupt_ack(m68k::cpu_bus& bus, std::uint8_t priority) override
	{
		this->priority = priority;
		bus.interrupt_priority(0);
	}

	m68k::interrupt_type interrupt_type() const override
	{
		return m68k::interrupt_type::autovectored;
	}

	std::uint8_t vector_number() const override
	{
		return autovectored(priority);
	}

private:
	std::uint8_t priority;
};

} // namespace genesis::m68k

#endif // __M68K_INTERRUPTING_DEVICE_H__
