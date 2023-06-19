#include "vdp.h"

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
				regs.control.set_c1(data);
			else
				regs.control.set_c2(data);
		}

		write_req.reset();

		return;
	}

	// TODO: check FIFO

	// check read pre-cache operation is required
	if(pre_cache_read_is_required())
	{
		// TODO: primitive implementation
		// TODO: advance address after read operation

		std::uint32_t address = regs.control.address();

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
	}
}


bool vdp::pre_cache_read_is_required() const
{
	// TODO: check FIFO is empty!

	if(regs.control.dma_enabled())
	{
		// current operation should be handled by DMA
		return false;
	}

	if(regs.control.work_completed())
	{
		// wait till old data is read
		return false;
	}

	if(regs.control.control_type() != control_type::read)
	{
		return false;
	}

	if(regs.control.vmem_type() == vmem_type::invalid)
	{
		// TODO: is it possible
		return false;
	}

	// TODO: what if vdp just started and control register is not set by ports?

	return true;
}


} // namespace genesis::vdp