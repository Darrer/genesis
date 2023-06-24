#ifndef __TEST_RANDOM_H__
#define __TEST_RANDOM_H__

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <vector>
#include <ctime>


namespace genesis::test
{

class random
{
	static bool inited;
public:
	template<class T>
	static T next()
	{
		init();

		// TODO: works only with unsigned numbers
		return std::rand() % std::numeric_limits<T>::max();
	}

	template<class T>
	static std::vector<T> next_few(unsigned count)
	{
		std::vector<T> res;
		for(unsigned i = 0; i < count; ++i)
			res.push_back(next<T>());
		res;
	}

private:
	static void init()
	{
		if(!inited)
		{
			std::srand((unsigned)std::time(0));
			inited = true;
		}
	}
};

};

#endif // __TEST_RANDOM_H__
