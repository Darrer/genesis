#ifndef __VDP_DMA_H__
#define __VDP_DMA_H__

#include <iostream>

#include "vdp/register_set.h"
#include "vdp/settings.h"
#include "memory_access.h"


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
		finishing,
	};

public:
	dma(vdp::register_set& regs, vdp::settings& sett, memory_access& memory)
		: regs(regs), sett(sett),  memory(memory) { }

	bool is_idle() const { return _state == state::idle; }
	
	void cycle()
	{
		check_work();

		switch (_state)
		{
		case state::idle:
			break;

		case state::fill_pending:
		{
			if(regs.fifo.empty())
			{
				// wait till FIFO gets an entry
				break;
			}

			_state = state::fill;
			break; 
		}

		case state::fill:
			do_fill();
			break;

		case state::vram_copy:
			do_vram_copy();
			break;

		// TODO: we're losing 1 cycle here
		case state::finishing:
		{
			do_finishing();
			break;
		}
		
		default: throw internal_error();
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

		// start dma
		switch (sett.dma_mode())
		{
		case dma_mode::vram_fill:
			_state = state::fill_pending;
			break;

		case dma_mode::vram_copy:
			_state = state::vram_copy;
			break;

		case dma_mode::mem_to_vram:
			throw not_implemented();
			break;
		
		default: throw internal_error();
		}
	}

	void do_fill()
	{
		if(regs.fifo.empty() == false)
		{
			// wait till VDP free the FIFO
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
		// TODO: not sure we have to wait FIFO
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
		std::uint16_t source = sett.dma_source();

		memory.init_read_vram(source);
		reading = true;

		sett.dma_source(source + 1);
	}

	void do_finishing()
	{
		if(memory.is_idle())
		{
			_state = state::idle;
			regs.control.dma_start(false);
		}
	}

	void advance()
	{
		// inc address
		regs.control.address( regs.control.address() + sett.auto_increment_value() );

		// dec length
		std::uint16_t length = sett.dma_length() - 1;
		sett.dma_length(length);

		if(length == 0)
		{
			// we're done, terminate dma operation
			_state = state::finishing;
		}
	}

private:
	vdp::register_set& regs;
	vdp::settings& sett;

	state _state = state::idle;
	memory_access& memory;
	bool reading = false;
};

};

#endif // __VDP_DMA_H__
