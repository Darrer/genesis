#include "random.h"

using namespace genesis;

std::mt19937 test::random::gen(std::random_device{}());
std::uniform_int_distribution<int> test::random::dist(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
