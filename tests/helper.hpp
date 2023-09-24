#ifndef __HELPER_HPP__
#define __HELPER_HPP__

#include <filesystem>
#include <gtest/gtest.h>
#include <chrono>


[[maybe_unused]] static std::filesystem::path get_exec_path()
{
	// I'm sorry for that...
	std::filesystem::path exec_path = testing::internal::GetArgvs()[0];
	exec_path.remove_filename();

	return std::filesystem::absolute(exec_path);
}

template<class Callable>
static long long measure_in_ns(const Callable& fn)
{
	auto start = std::chrono::high_resolution_clock::now();

	fn();

	auto stop = std::chrono::high_resolution_clock::now();

	auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
	return dur.count();
}

#endif // __HELPER_HPP__
