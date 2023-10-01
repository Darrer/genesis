#ifndef __TEST_HELPER_H__
#define __TEST_HELPER_H__

#include "../helpers/random.h"
#include "memory/addressable.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <map>


namespace genesis::test
{

template <class T>
	requires(sizeof(T) <= 2)
void test_read_write(genesis::memory::addressable& unit)
{
	const std::uint32_t max_address = unit.capacity() - sizeof(T);

	// not the optimal way to store written data, but the easiest one
	std::map<std::uint32_t /* address */, T /* data */> written_data;

	/* write random data */
	for(std::uint32_t addr = 0; addr <= max_address; addr += sizeof(T))
	{
		T data = test::random::next<T>();
		unit.init_write(addr, data);
		written_data[addr] = data;
	}

	/* check written data */
	for(std::uint32_t addr = 0; addr <= max_address; addr += sizeof(T))
	{
		T data{};
		if constexpr(sizeof(T) == 1)
		{
			unit.init_read_byte(addr);
			data = unit.latched_byte();
		}
		else if constexpr(sizeof(T) == 2)
		{
			unit.init_read_word(addr);
			data = unit.latched_word();
		}

		T expected_data = written_data[addr];
		ASSERT_EQ(expected_data, data);
	}

	// self-test
	const auto expected_writes = sizeof(T) == 1 ? unit.capacity() : unit.capacity() / 2;
	ASSERT_EQ(expected_writes, written_data.size());
}

} // namespace genesis::test

#endif // __TEST_HELPER_H__
