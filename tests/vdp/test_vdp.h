#ifndef __TEST_VDP_H__
#define __TEST_VDP_H__

#include "vdp/vdp.h"
#include "exception.hpp"

namespace genesis::test
{

class vdp : public genesis::vdp::vdp
{
static const std::uint32_t cycle_limit = 100'000;

public:
	std::uint32_t wait_fifo()
	{
		std::uint32_t cycles = 0;
		while(!registers().fifo.empty())
		{
			cycle();
			++cycles;

			if(cycles == cycle_limit)
				throw internal_error();
		}

		return cycles;
	}

	std::uint32_t wait_io_ports()
	{
		std::uint32_t cycles = 0;
		while(!io_ports().is_idle())
		{
			cycle();
			++cycles;

			if(cycles == cycle_limit)
				throw internal_error();
		}

		return cycles;
	}
};

}

#endif // __TEST_VDP_H__