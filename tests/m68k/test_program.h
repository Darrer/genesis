#ifndef __M68K_TEST_PROGRAMM_H__
#define __M68K_TEST_PROGRAMM_H__

#include "MCL/mcl.h"

namespace genesis::test
{

// return true if test program completed successfully
template <class Callable>
bool run_test_program(test_cpu& cpu, Callable&& after_cycle_hook)
{
	// use MCL as a default test program
	return run_mcl(cpu, after_cycle_hook);
}

// it safe to use memory after this address
const std::uint32_t address_after_test_programm = 0x3c0000;

} // namespace genesis::test

#endif // __M68K_TEST_PROGRAMM_H__
