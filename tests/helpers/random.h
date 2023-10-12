#ifndef __TEST_RANDOM_H__
#define __TEST_RANDOM_H__

#include <limits>
#include <vector>
#include <random>


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

private:
	static std::mt19937 gen;
	static std::uniform_int_distribution<int> dist;
};

};

#endif // __TEST_RANDOM_H__
