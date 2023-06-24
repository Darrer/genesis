#include "vdp.h"

#include <iostream>

namespace genesis::vdp
{

vdp::vdp() : _sett(regs), ports(regs), _vram(std::make_unique<vram_t>())
{
}

void vdp::cycle()
{
	ports.cycle();
	handle_ports_requests();
}

void vdp::handle_ports_requests()
{
	auto& write_req = ports.pending_control_write_requet();
	if(write_req.has_value())
	{
		// perform the request
		std::uint16_t data = write_req.value().data;
		bool first_word = write_req.value().first_word;

		// TODO: for now write directly to registers
		if(first_word && (data >> 14) == 0b10)
		{
			// write data to register
			std::uint8_t reg_data = data & 0xFF;
			std::uint8_t reg_num = (data >> 8) & 0b11111;
			if(reg_num <= 23)
			{
				regs.set_register(reg_num, reg_data);
			}
		}
		else
		{
			if(first_word)
			{
				regs.control.set_c1(data);
			}
			else
			{
				bool dma_start_flag = regs.control.dma_start();

				regs.control.set_c2(data);

				if(_sett.dma_enabled() == false)
				{
					// writing to control port cannot change CD5 bit if DMA is disabled
					// so restore old value
					regs.control.dma_start(dma_start_flag);
				}
			}
		}

		write_req.reset();

		return;
	}

	if(!regs.fifo.empty())
	{
		auto entry = regs.fifo.pop();

		switch (entry.control.vmem_type())
		{
		case vmem_type::vram:
		{
			if(entry.control.address() % 2 == 1)
			{
				// writing to odd addresses swaps bytes
				endian::swap(entry.data);

				// writing cannot cross a word boundary
				entry.control.address( entry.control.address() & ~1 );
			}
			_vram->write(entry.control.address(), entry.data);
		}
			break;

		case vmem_type::cram:
			_cram.write(entry.control.address(), entry.data);
			break;

		case vmem_type::vsram:
			_vsram.write(entry.control.address(), entry.data);
			break;
		
		default: throw internal_error();
		}

		return;
	}

	// check read pre-cache operation is required
	if(pre_cache_read_is_required())
	{
		// TODO: primitive implementation
		// TODO: advance address after read operation

		std::uint32_t address = regs.control.address() & ~1; // ignore 0bit

		switch (regs.control.vmem_type())
		{
		case vmem_type::vram:
		{
			std::uint8_t lsb = _vram->read<std::uint8_t>(address);
			std::uint8_t msb = _vram->read<std::uint8_t>(address + 1);

			regs.read_cache.set_lsb(lsb);
			regs.read_cache.set_msb(msb);

			regs.control.work_completed(true);

			break;
		}

		case vmem_type::cram:
		{
			// TODO: we read like 9 bits only, the other 7 bits should be got from FIFO (or smth???)
			std::uint16_t data = _cram.read(address);

			regs.read_cache.set(data);
			regs.control.work_completed(true);

			break;
		}

		case vmem_type::vsram:
		{
			std::uint16_t data = _vsram.read(address);

			regs.read_cache.set(data);
			regs.control.work_completed(true);

			break;
		}

		default: throw internal_error();
		}

		return;
	}
}


bool vdp::pre_cache_read_is_required() const
{
	if(!regs.fifo.empty())
	{
		// FIFO has priority over read pre-cache
		return false;
	}

	if(regs.control.dma_start()) // TODO: fixme
	{
		// current operation should be handled by DMA
		return false;
	}

	if(regs.control.work_completed())
	{
		// wait till pre-read data is grabbed
		return false;
	}

	if(regs.control.control_type() != control_type::read)
	{
		return false;
	}

	if(regs.control.vmem_type() == vmem_type::invalid)
	{
		// TODO: is it possible?
		return false;
	}

	// TODO: what if vdp just started and control register is not set by ports?

	return true;
}


} // namespace genesis::vdp