#ifndef __VDP_DMA_H__
#define __VDP_DMA_H__

#include "memory_access.h"
#include "vdp/m68k_bus_access.h"
#include "vdp/register_set.h"
#include "vdp/settings.h"

#include <iostream>
#include <memory>


namespace genesis::vdp::impl
{

class dma
{
private:
	enum class state
	{
		idle,
		fill_pending,
		fill,
		vram_copy,
		m68k_copy,
		finishing,
	};

public:
	dma(vdp::register_set& regs, vdp::settings& sett, memory_access& memory,
		std::shared_ptr<vdp::m68k_bus_access> m68k_bus)
		: regs(regs), sett(sett), memory(memory), m68k_bus(m68k_bus)
	{
	}

	bool is_idle() const
	{
		return _state == state::idle;
	}

	void set_m68k_bus_access(std::shared_ptr<vdp::m68k_bus_access> m68k_bus)
	{
		this->m68k_bus = m68k_bus;
	}

	void cycle()
	{
		check_work();

		switch(_state)
		{
		case state::idle:
			break;

		case state::fill_pending: {
			if(regs.fifo.empty())
			{
				// wait till FIFO gets an entry
				break;
			}

			// TODO: should we start processing operation on this cycle?
			_state = state::fill;
			break;
		}

		case state::fill:
			do_fill();
			break;

		case state::vram_copy:
			do_vram_copy();
			break;

		case state::m68k_copy:
			do_m68k_copy();
			break;

		// TODO: we're losing 1 cycle here
		case state::finishing: {
			do_finishing();
			break;
		}

		default:
			throw internal_error();
		}
	}

private:
	void check_work()
	{
		if(_state != state::idle)
			return;

		if(regs.control.dma_start() == false)
		{
			// dma enabled, but we've got nothing to do
			return;
		}

		// TODO: reset DMA unit to make sure the previous operation won't affect the new one

		// start dma
		switch(sett.dma_mode())
		{
		case dma_mode::vram_fill:
			_state = state::fill_pending;
			break;

		case dma_mode::vram_copy:
			_state = state::vram_copy;
			break;

		case dma_mode::mem_to_vram:
			_state = state::m68k_copy;
			break;

		default:
			throw internal_error();
		}

		m_length = sett.dma_length();
		m_source = sett.dma_source();
		regs.SR.DMA = 1;
	}

	void do_fill()
	{
		if(regs.fifo.empty() == false)
		{
			// wait till VDP frees the FIFO
			return;
		}

		if(memory.is_idle() == false)
		{
			// wait till prevous write operation is over
			return;
		}

		auto addr = regs.control.address();

		auto mem_type = regs.control.vmem_type();
		if(mem_type == vmem_type::invalid)
			throw not_implemented();

		std::uint16_t fill_data;
		if(mem_type == vmem_type::vram)
			fill_data = data_to_fill_vram();
		else
			fill_data = regs.fifo.next().data;

		memory.init_write(mem_type, addr, fill_data);

		advance();
	}

	std::uint8_t data_to_fill_vram() const
	{
		std::uint16_t data = regs.fifo.prev().data;
		return endian::msb(data);
	}

	void do_vram_copy()
	{
		// TODO: not sure if we have to wait FIFO
		// TODO: VDP should stop processing requests from CPU during DMA VRAM copy
		if(regs.fifo.empty() == false)
		{
			// wait till VDP free the FIFO
			return;
		}

		if(memory.is_idle() == false)
		{
			// wait till prevous operation is over
			return;
		}

		// try write first
		if(reading)
		{
			reading = false; // memory is idle, we're not reading anymore, result should be available

			memory.init_write(vmem_type::vram, regs.control.address(), memory.latched_byte());
			advance();

			return;
		}

		// read next byte
		std::uint16_t source = dma_source();
		advance_dma_source(+1);

		memory.init_read_vram(source);
		reading = true;
	}

	void do_m68k_copy()
	{
		if(regs.fifo.full())
		{
			// wait till we get at least 1 slot
			return;
		}

		if(m68k_bus->is_idle() == false)
		{
			// wait till current operation is over
			return;
		}

		if(access_requested == false)
		{
			m68k_bus->request_bus();
			access_requested = true;
			return;
		}

		if(m68k_bus->bus_granted() == false)
		{
			// wait till get the bus
			return;
		}

		if(reading)
		{
			reading = false;

			std::uint16_t data = m68k_bus->latched_word();
			regs.fifo.push(data, regs.control);
			inc_control_address();

			// TODO: abort DMA if transfer is to CRAM/VSRAM and address exceeds max possible address
			if(dma_length() == 0)
			{
				// we're done, terminate dma operation
				m68k_bus->release_bus();
				access_requested = false;

				_state = state::finishing;
				return;
			}
		}

		std::uint32_t src_address = dma_source();
		advance_dma_source(+2);

		dec_length();

		m68k_bus->init_read_word(src_address);
		reading = true;
	}

	void do_finishing()
	{
		// Ensure all operations are complete before transitioning to idle
		if(memory.is_idle() && m68k_bus->is_idle())
		{
			_state = state::idle;
			regs.control.dma_start(false);
			regs.SR.DMA = 0;
		}
		// If not idle, continue waiting - this ensures we don't lose cycles
	}

	void inc_control_address()
	{
		regs.control.address(regs.control.address() + sett.auto_increment_value());
	}

	void advance()
	{
		inc_control_address();

		dec_length();

		if(dma_length() == 0)
		{
			// we're done, terminate dma operation
			_state = state::finishing;
		}
	}

	std::uint16_t dma_length() const
	{
		return m_length;
	}

	void dec_length()
	{
		--m_length;
		sett.dma_length(sett.dma_length() - 1);
	}

	std::uint32_t dma_source() const
	{
		return m_source;
	}

	void advance_dma_source(std::uint32_t offset)
	{
		m_source += offset;
		sett.dma_source(sett.dma_source() + offset);
	}

private:
	state _state = state::idle;

	vdp::register_set& regs;
	vdp::settings& sett;

	memory_access& memory;
	bool reading = false;

	std::shared_ptr<vdp::m68k_bus_access> m68k_bus;
	bool access_requested = false;

	std::uint16_t m_length = 0;
	std::uint32_t m_source = 0;
};

}; // namespace genesis::vdp::impl

#endif // __VDP_DMA_H__
