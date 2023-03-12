#ifndef __M68K_TEST_CPU_HPP__
#define __M68K_TEST_CPU_HPP__

#include "m68k/cpu.h"

namespace genesis::test
{

class test_cpu : public genesis::m68k::cpu
{
public:
	test_cpu() : cpu(std::make_shared<m68k::memory>()) { }

	m68k::memory& memory() { return *mem; }
	m68k::bus_manager& bus_manager() { return *busm; }
	m68k::prefetch_queue& prefetch_queue() { return *pq; }
};

}


#endif // __M68K_TEST_CPU_HPP__
