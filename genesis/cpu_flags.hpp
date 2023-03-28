#ifndef __GENESIS_CPU_FLAGS_HPP__
#define __GENESIS_CPU_FLAGS_HPP__

#include <cstdint>
#include <limits>


namespace genesis
{

class cpu_flags
{
public:
	cpu_flags() = delete;

	// check if a + b leads to overflow
	template <class T>
	static std::uint8_t overflow_add(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);
		static_assert(sizeof(T) <= 4);

		auto sum = (std::int64_t)a + (std::int64_t)b + c;

		if (sum > std::numeric_limits<T>::max())
			return 1;

		if (sum < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	// check if a - b leads to overflow
	template <class T>
	static std::uint8_t overflow_sub(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);
		static_assert(sizeof(T) <= 4);

		auto diff = (std::int64_t)a - (std::int64_t)b - c;

		if (diff > std::numeric_limits<T>::max())
			return 1;

		if (diff < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	// check if a + b leads to carry
	template <class T>
	static std::uint8_t carry(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);
		static_assert(sizeof(T) <= 4);

		auto sum = (std::int64_t)a + (std::int64_t)b + c;
		if (sum > std::numeric_limits<T>::max())
			return 1;

		return 0;
	}

	// check if a - b leads to borrow
	template <class T>
	static std::uint8_t borrow(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);
		static_assert(sizeof(T) <= 4);

		auto sum = (std::int64_t)b + c;
		if (sum > a)
			return 1;

		return 0;
	}
};

}

#endif // __GENESIS_CPU_FLAGS_HPP__
