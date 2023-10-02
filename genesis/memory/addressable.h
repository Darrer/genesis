#ifndef __ADDRESSABLE_H__
#define __ADDRESSABLE_H__

#include <cstdint>

namespace genesis::memory
{

class addressable
{
public:
	virtual ~addressable() = default;

	virtual std::uint32_t max_address() const = 0;

	virtual bool is_idle() const = 0;

	virtual void init_write(std::uint32_t address, std::uint8_t data) = 0;
	virtual void init_write(std::uint32_t address, std::uint16_t data) = 0;

	virtual void init_read_byte(std::uint32_t address) = 0;
	virtual void init_read_word(std::uint32_t address) = 0;

	virtual std::uint8_t latched_byte() const = 0;
	virtual std::uint16_t latched_word() const = 0;
};

}; // namespace genesis::memory

#endif // __ADDRESSABLE_H__
