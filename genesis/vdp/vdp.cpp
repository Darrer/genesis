#include "vdp.h"

#include <iostream>

namespace genesis::vdp
{

vdp::vdp(std::shared_ptr<m68k_bus_access> m68k_bus)
	: _sett(regs), ports(regs), dma(regs, _sett, dma_memory, m68k_bus)
{
}

void vdp::cycle()
{
	ports.cycle();

	if(_sett.dma_enabled())
		dma.cycle();

	// io ports have priority over DMA
	handle_ports_requests();

	// TODO: can we execute io ports and DMA requests in 1 cycle?
	handle_dma_requests();
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

		switch(entry.control.vmem_type())
		{
		case vmem_type::vram: {
			if(entry.control.address() % 2 == 1)
			{
				// writing to odd addresses swaps bytes
				endian::swap(entry.data);

				// writing cannot cross a word boundary
				entry.control.address(entry.control.address() & ~1);
			}

			// TODO: vram has byte-only access
			// std::cout << "Writing " << entry.data << " at " << entry.control.address() << std::endl;
			_vram.write(entry.control.address(), entry.data);
		}
		break;

		case vmem_type::cram:
			_cram.write(entry.control.address(), entry.data);
			break;

		case vmem_type::vsram:
			_vsram.write(entry.control.address(), entry.data);
			break;

		default:
			throw internal_error();
		}

		return;
	}

	// check read pre-cache operation is required
	if(pre_cache_read_is_required())
	{
		// TODO: primitive implementation
		// TODO: advance address after read operation

		std::uint32_t address = regs.control.address() & ~1; // ignore 0bit

		switch(regs.control.vmem_type())
		{
		case vmem_type::vram: {
			std::uint8_t lsb = _vram.read<std::uint8_t>(address);
			std::uint8_t msb = _vram.read<std::uint8_t>(address + 1);

			regs.read_cache.set_lsb(lsb);
			regs.read_cache.set_msb(msb);

			regs.control.work_completed(true);

			break;
		}

		case vmem_type::cram: {
			// TODO: we read like 9 bits only, the other 7 bits should be got from FIFO (or smth???)
			std::uint16_t data = _cram.read(address);

			regs.read_cache.set(data);
			regs.control.work_completed(true);

			break;
		}

		case vmem_type::vsram: {
			std::uint16_t data = _vsram.read(address);

			regs.read_cache.set(data);
			regs.control.work_completed(true);

			break;
		}

		default:
			throw internal_error();
		}

		return;
	}
}

void vdp::handle_dma_requests()
{
	if(!_sett.dma_enabled())
		return;

	auto& write_req = dma_memory.pending_write();
	if(write_req.has_value())
	{
		auto req = write_req.value();
		switch(req.type)
		{
		case vmem_type::vram:
			vram_write(req.address, std::uint8_t(req.data));
			write_req.reset();
			break;

		case vmem_type::cram:
			cram_write(req.address, req.data);
			write_req.reset();
			break;

		case vmem_type::vsram:
			vsram_write(req.address, req.data);
			write_req.reset();
			break;

		default:
			throw internal_error();
		}
	}

	auto& read_req = dma_memory.pending_read();
	if(read_req.has_value())
	{
		std::uint8_t data = _vram.read<std::uint8_t>(read_req.value().address);
		read_req.reset();
		dma_memory.set_read_result(data);
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


void vdp::vram_write(std::uint32_t address, std::uint8_t data)
{
	_vram.write(address, data);
}

void vdp::cram_write(std::uint32_t address, std::uint16_t data)
{
	_cram.write(address, data);
}

void vdp::vsram_write(std::uint32_t address, std::uint16_t data)
{
	_vsram.write(address, data);
}


} // namespace genesis::vdp