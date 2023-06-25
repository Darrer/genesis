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
		return wait([this](){ return registers().fifo.empty(); });
	}

	std::uint32_t wait_io_ports()
	{
		return wait([this](){ return io_ports().is_idle(); });
	}

	std::uint32_t wait_write()
	{
		std::uint32_t cycles = 0;
		cycles += wait_io_ports(); // first wait ports
		cycles += wait_fifo(); // then make sure vdp wrote the data
		return cycles;
	}

	std::uint32_t wait_dma()
	{
		// TODO: increase cycle threshold
		return wait([this]() { return dma.is_idle(); } );
	}

private:
	template<class Callable>
	std::uint32_t wait(const Callable&& predicate)
	{
		std::uint32_t cycles = 0;
		while (!predicate())
		{
			cycle();
			++cycles;

			if(cycles == cycle_limit)
				throw internal_error("wait: exceed limit");
		}

		return cycles;
	}
};

}

#endif // __TEST_VDP_H__