#ifndef __MEMORY_MEMORY_UNIT_H__
#define __MEMORY_MEMORY_UNIT_H__

#include "addressable.h"
#include "base_unit.h"
#include "endian.hpp"
#include "exception.hpp"
#include "string_utils.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace genesis::memory
{

/* Generic memory unit used to represent ROM and M68K/Z80 RAM */

class memory_unit : public base_unit
{
public:
	/* in bytes [0 ; highest_address] */
	memory_unit(std::uint32_t highest_address, std::endian byte_order = std::endian::native)
		: memory_unit(create_buffer(highest_address), byte_order)
	{
	}

	memory_unit(std::shared_ptr<std::vector<std::uint8_t>> buffer, std::endian byte_order = std::endian::native)
		: base_unit(std::span<std::uint8_t>(buffer->data(), buffer->size()), byte_order), m_buffer(buffer)
	{
	}

private:
	static std::shared_ptr<std::vector<std::uint8_t>> create_buffer(std::uint32_t highest_address)
	{
		auto buffer = std::make_shared<std::vector<std::uint8_t>>();
		buffer->resize(highest_address + 1); // + 1 to account for 0 index
		return buffer;
	}

private:
	std::shared_ptr<std::vector<std::uint8_t>> m_buffer;
};

[[maybe_unused]] [[nodiscard]]
static std::unique_ptr<memory::addressable> make_memory_unit(std::uint32_t highest_address,
															 std::endian byte_order = std::endian::native)
{
	return std::make_unique<memory::memory_unit>(highest_address, byte_order);
}

} // namespace genesis::memory

#endif // __MEMORY_MEMORY_UNIT_H__
