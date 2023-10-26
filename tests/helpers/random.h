#ifndef __TEST_RANDOM_H__
#define __TEST_RANDOM_H__

#include <limits>
#include <vector>
#include <random>
#include <stdexcept>


namespace genesis::test
{

class random
{
	static bool inited;
public:
	template<class T>
	static T next()
	{
		// TODO: works only with unsigned numbers
		static_assert(std::is_unsigned_v<T>);
		return dist(gen) % std::numeric_limits<T>::max();
	}

	template<class T>
	static std::vector<T> next_few(unsigned count)
	{
		std::vector<T> res;
		for(unsigned i = 0; i < count; ++i)
			res.push_back(next<T>());
		return res;
	}

	// returns random value in range [a; b]
	template<class T>
	static T in_range(T a, T b)
	{
		if(b <= a)
			throw std::invalid_argument("a must be less then b");

		T diff = b - a;
		return a + (next<T>() % (diff + 1));
	}

	template<class T>
	static T::value_type& pick(T& array)
	{
		if(array.size() == 0)
			throw std::invalid_argument("array should not be empty");

		if(array.size() == 1)
			return array[0];

		auto idx = in_range<typename T::size_type>(0, array.size() - 1);
		return array[idx];
	}

private:
	static std::mt19937 gen;
	static std::uniform_int_distribution<int> dist;
};

};

#endif // __TEST_RANDOM_H__
