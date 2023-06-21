#ifndef __TEST_RANDOM_H__
#define __TEST_RANDOM_H__

#include <cstdint>
#include <limits>
#include <cstdlib>


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

		// TODO: works only for unsigned numbers
		return std::rand() % std::numeric_limits<T>::max();
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

bool random::inited = false;

};

#endif // __TEST_RANDOM_H__
