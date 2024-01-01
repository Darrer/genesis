#include "random.h"
#include <iostream>

using namespace genesis;

std::random_device::result_type seed()
{
	auto seed = std::random_device{}();
	std::cout << "Random seed: " << seed << '\n';
	return seed;
}

std::mt19937 test::random::gen(seed());
std::uniform_int_distribution<int> test::random::dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
