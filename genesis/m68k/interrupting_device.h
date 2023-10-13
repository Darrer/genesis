#ifndef __M68K_INTERRUPTING_DEVICE_H__
#define __M68K_INTERRUPTING_DEVICE_H__

#include <cstdint>

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
	virtual void init_interrupt_ack(std::uint8_t priority) = 0;

	virtual m68k::interrupt_type interrupt_type() = 0;
	virtual std::uint8_t vector_number() = 0;

protected:
	/* helpers */
	std::uint8_t autovectored(std::uint8_t priority)
	{
		return 0x18 + priority;
	}
};

}

#endif // __M68K_INTERRUPTING_DEVICE_H__