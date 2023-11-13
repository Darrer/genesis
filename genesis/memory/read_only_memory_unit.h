#ifndef __READ_ONLY_MEMORY_UNIT_H__
#define __READ_ONLY_MEMORY_UNIT_H__

#include "memory_unit.h"
#include "exception.hpp"

namespace genesis::memory
{

class read_only_memory_unit : public memory_unit
{
public:
	using memory_unit::memory_unit;

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		throw internal_error();
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		throw internal_error();
	}
};

}

#endif // __READ_ONLY_MEMORY_UNIT_H__
