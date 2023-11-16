#ifndef __READ_ONLY_MEMORY_UNIT_H__
#define __READ_ONLY_MEMORY_UNIT_H__

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
		rise_access_violation(address);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		rise_access_violation(address);
	}

private:
	static void rise_access_violation(std::uint32_t address)
	{
		throw std::runtime_error("Access violation: Attempt to write to read-only memory at address "
			+ su::hex_str(address));
	}
};

}

#endif // __READ_ONLY_MEMORY_UNIT_H__
