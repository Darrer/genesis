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

	template<class T>
	static std::vector<T> next_few_in_range(unsigned count, T a, T b)
	{
		std::vector<T> res;
		for(unsigned i = 0; i < count; ++i)
			res.push_back(in_range<T>(a, b));
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

	template<class T>
	static const T pick(std::initializer_list<T> array)
	{
		if(array.size() == 0)
			throw std::invalid_argument("array should not be empty");

		if(array.size() == 1)
			return *array.begin();

		auto idx = in_range<typename std::initializer_list<T>::size_type>(0, array.size() - 1);
		auto it = array.begin();
		std::advance(it, idx);
		return *it;
	}

	static const bool is_true()
	{
		return next<unsigned>() % 2 == 0;
	}

private:
	static std::mt19937 gen;
	static std::uniform_int_distribution<int> dist;
};

};

#endif // __TEST_RANDOM_H__
