#ifndef __GENESIS_TIME_UTILS_H__
#define __GENESIS_TIME_UTILS_H__

#include <chrono>

namespace genesis::time
{

template <class Callable>
static long long measure_in_ms(Callable func)
{
	auto start = std::chrono::high_resolution_clock::now();

	func();

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
	return dur.count();
}

template <class Callable>
static long long measure_in_ns(Callable func)
{
	auto start = std::chrono::high_resolution_clock::now();

	func();

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	return dur.count();
}

};

#endif // __GENESIS_TIME_UTILS_H__
