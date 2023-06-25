#ifndef __VDP_DMA_H__
#define __VDP_DMA_H__

#include <iostream>

#include "vdp/register_set.h"
#include "vdp/settings.h"
#include <optional>

namespace genesis::vdp::impl
{

struct pending_write
{
	vmem_type type;
	std::uint32_t address;
	std::uint16_t data;
};

class dma
{
private:
	enum class state
	{
		idle,
		fill_pending,
		fill,
		finishing,
	};

public:
	dma(vdp::register_set& regs, vdp::settings& sett) : regs(regs), sett(sett) { }

	bool is_idle() const { return _state == state::idle; }
	
	void cycle()
	{
		check_start();

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
		{
			do_fill();
			break;
		}

		// TODO: we're losing 1 cycle here
		case state::finishing:
		{
			if(write_is_done())
			{
				_state = state::idle;
				regs.control.dma_start(false);
			}

			break;
		}
		
		default: throw internal_error();
		}
	}

	// TODO: refactor read/write interface
	std::optional<pending_write>& pending_write() { return write_req; }

private:
	void check_start()
	{
		if(_state != state::idle || _state == state::finishing)
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
		
		default: throw not_implemented();
		}
	}

	void do_fill()
	{
		if(regs.fifo.empty() == false)
		{
			// wait till VDP free the FIFO
			return;
		}

		if(write_is_done() == false)
		{
			// wait till prevous write operation is over
			return;
		}

		// only work with vram for now
		if(regs.control.vmem_type() != vmem_type::vram)
			throw not_implemented();

		std::uint8_t fill_data = data_to_fill_vram();

		auto addr = regs.control.address();

		// std::cout << "Writing to " << addr << " : " << fill_data << std::endl;
		init_write(vmem_type::vram, addr, fill_data);

		regs.control.address( regs.control.address() + sett.auto_increment_value() );

		// update dma length
		std::uint16_t length = sett.dma_length() - 1;
		sett.dma_length(length);
		if(length == 0)
		{
			_state = state::finishing;
		}
	}

	std::uint8_t data_to_fill_vram()
	{
		std::uint16_t data = regs.fifo.prev().data;
		return endian::msb(data);
	}

private:
	template<class T>
	void init_write(vmem_type type, std::uint32_t address, T data)
	{
		if(write_is_done() == false)
			throw internal_error();

		write_req = { type, address, std::uint16_t(data) };
	}

	bool write_is_done() const { return write_req.has_value() == false; }

private:
	vdp::register_set& regs;
	vdp::settings& sett;

	state _state = state::idle;
	std::optional<struct pending_write> write_req;
};

};

#endif // __VDP_DMA_H__
