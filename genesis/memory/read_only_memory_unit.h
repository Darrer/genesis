#ifndef __MEMORY_READ_ONLY_MEMORY_UNIT_H__
#define __MEMORY_READ_ONLY_MEMORY_UNIT_H__

#include "memory_unit.h"
#include "exception.hpp"
#include "string_utils.hpp"

namespace genesis::memory
{

class read_only_memory_unit : public memory_unit
{
public:
	using memory_unit::memory_unit;

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		rise_access_violation(address, data);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		rise_access_violation(address, data);
	}

private:
	template<class T>
	static void rise_access_violation(std::uint32_t address, T data)
	{
		throw std::runtime_error("Access violation: Attempt to write to read-only memory at address "
			+ su::hex_str(address) + " (data " + su::hex_str(data) + ")");
	}
};

}

#endif // __MEMORY_READ_ONLY_MEMORY_UNIT_H__
