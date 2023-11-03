#ifndef __TEST_VDP_H__
#define __TEST_VDP_H__

#include "memory/memory_unit.h"
#include "vdp/vdp.h"
#include "exception.hpp"
#include "vdp/m68k_bus_access.h"

namespace genesis::test
{

class mock_m68k_bus_access : public m68k_bus_access
{
public:
	using m68k_memory_t = memory::memory_unit;

public:
	mock_m68k_bus_access() : m68k_memory(0xFFFF, std::endian::big)
	{
	}

	void request_bus() override
	{
		assert_idle();

		has_access = true;
		cycles_to_idle = 10;
	}

	void release_bus() override
	{
		assert_access();
		assert_idle();

		has_access = false;
		cycles_to_idle = 10;
	}

	void init_read_word(std::uint32_t address) override
	{
		assert_access();
		assert_idle();

		data = m68k_memory.read<std::uint16_t>(address);
		cycles_to_idle = 10;
	}

	std::uint16_t latched_word() const override
	{
		return data.value();
	}

	bool is_idle() const override { return cycles_to_idle == 0; }

	m68k_memory_t& memory() { return m68k_memory; }
	bool bus_acquired() const { return has_access; }

	void cycle()
	{
		if(cycles_to_idle > 0)
			--cycles_to_idle;
	}

private:
	void assert_idle()
	{
		if(!is_idle())
			throw internal_error();
	}

	void assert_access()
	{
		if(!has_access)
			throw internal_error();
	}

private:
	bool has_access = false;
	std::optional<std::uint16_t> data = 0;

	memory::memory_unit m68k_memory;

	int cycles_to_idle = 0;
};


class vdp : public genesis::vdp::vdp
{
static const std::uint32_t cycle_limit = 100'000;

	vdp(std::shared_ptr<mock_m68k_bus_access> m68k_bus) : genesis::vdp::vdp(m68k_bus), m68k_bus(m68k_bus) { }

public:
	vdp() : vdp (std::make_shared<mock_m68k_bus_access>()) { }

	vdp(std::shared_ptr<genesis::m68k_bus_access> _m68k_bus) : genesis::vdp::vdp(_m68k_bus) { }

	void cycle()
	{
		if(m68k_bus)
			m68k_bus->cycle();
		genesis::vdp::vdp::cycle();
	}

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

	std::uint32_t wait_dma_start()
	{
		return wait([this]() { return !dma.is_idle(); });
	}

	mock_m68k_bus_access& m68k_bus_access()
	{
		if(m68k_bus == nullptr)
			throw internal_error();
		return *m68k_bus;
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

private:
	std::shared_ptr<mock_m68k_bus_access> m68k_bus;
};

}

#endif // __TEST_VDP_H__