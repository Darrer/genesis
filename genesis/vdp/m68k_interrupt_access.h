#ifndef __VDP_M68K_INTERRUPT_ACCESS_H__
#define __VDP_M68K_INTERRUPT_ACCESS_H__

#include <functional>

namespace genesis::vdp
{

class m68k_interrupt_access
{
public:
	virtual ~m68k_interrupt_access() = default;

	virtual void interrupt_priority(std::uint8_t ipl) = 0;
	virtual std::uint8_t interrupt_priority() const = 0;

	using interrupt_callback = std::function<void(std::uint8_t /* current IPL */)>;
	virtual void set_interrupt_callback(interrupt_callback) = 0;
};

} // namespace genesis::vdp

#endif // __VDP_M68K_INTERRUPT_ACCESS_H__
