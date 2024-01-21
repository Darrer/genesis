#include "vdp.h"

#include <iostream>

namespace genesis::vdp
{

const bool PAL = true;
const genesis::vdp::mode MODE = genesis::vdp::mode::PAL;

// MCLK = 53203424 - 50hz
// MCLK = 53693175 - 60hz
enum class clock_rate
{
	hz50,
	hz60,
};

unsigned int cycles_per_line(settings& /* sett */)
{
	return 3420;
}

int cycles_per_pixel(settings& sett)
{
	if(sett.display_width() == display_width::c32)
		return 10; // 3420 / 342
	return 8; // 3420 / 420
}

int lines_per_frame(settings& sett, clock_rate rate = clock_rate::hz60)
{
	if(rate == clock_rate::hz50)
	{
		return 313;
	}
	else
	{
		if(sett.display_height() == display_height::c30)
			return 512;
		return 262;
	}
}

int pixels_per_line(settings& sett)
{
	if(sett.display_width() == display_width::c32)
		return 342;
	return 420;
}

vdp::vdp(std::shared_ptr<m68k_bus_access> m68k_bus)
	: _sett(regs), ports(regs), dma(regs, _sett, dma_memory, m68k_bus),
	  m_render(regs, _sett, _vram, _vsram, _cram)
{
}

void vdp::cycle()
{
	mclk++;

	update_hv_counters();

	if(mclk == 1)
	{
		on_start_scanline();
	}

	on_scanline();

	if(mclk == cycles_per_line(_sett))
	{
		on_end_scanline();
		m_scanline = (m_scanline + 1) % lines_per_frame(_sett);
		mclk = 0;
	}
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
			/* It seems that official/unofficial documentation is incorrect about write order.
			 * The following order seems the right one:
			 * If address is even: we first have to write MSB and then LSB.
			 * If address is odd : we first have to write LSB and then LSB.
			*/

			if(entry.control.address() % 2 == 1)
			{
				// writing to odd addresses swaps bytes
				endian::swap(entry.data);

				// writing cannot cross a word boundary
				entry.control.address(entry.control.address() & ~1);
			}

			// TODO: vram has byte-only access
			_vram.write(entry.control.address(), entry.data);
		}
		break;

		case vmem_type::cram:
			_cram.write(entry.control.address(), entry.data);
			break;

		case vmem_type::vsram:
			_vsram.write(entry.control.address(), entry.data);
			break;

		case vmem_type::invalid:
			// TODO: what should we do?
			throw genesis::not_implemented();

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
			std::uint16_t data = _vram.read<std::uint16_t>(address);
			regs.read_cache.set_lsb(endian::lsb(data));
			regs.read_cache.set_msb(endian::msb(data));

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

void vdp::update_status_register()
{
	regs.SR.E = regs.fifo.empty() ? 1 : 0;
	regs.SR.F = regs.fifo.full() ? 1 : 0;
	regs.SR.PAL = regs.R1.M2;
	regs.SR.VI = m_vint_pending ? 1 : 0;
}

void vdp::on_start_scanline()
{
	if(regs.v_counter == 0 || regs.SR.VB == 1 || hint_counter == 0)
	{
		hint_counter = _sett.horizontal_interrupt_counter();
	}
}

void vdp::on_end_scanline()
{
	// primitive approach
	int vint_threshold = _sett.display_height() == display_height::c28 ? 0xE0 : 0xF0;
	if(regs.v_counter == vint_threshold)
	{
		if(on_frame_end_callback != nullptr)
			on_frame_end_callback();

		m_render.reset_limits();
	}

	check_interrupts();
}

void vdp::on_scanline()
{
	ports.cycle();

	if(_sett.dma_enabled())
		dma.cycle();

	// io ports have priority over DMA
	handle_ports_requests();

	// TODO: can we execute io ports and DMA requests in 1 cycle?
	handle_dma_requests();

	update_status_register();
}

void vdp::check_interrupts()
{
	if(m_hint_pending && _sett.horizontal_interrupt_enabled())
	{
		if(m68k_int == nullptr)
			throw genesis::internal_error();
		// std::cout << "Raising HINT\n";
		m68k_int->interrupt_priority(4);
		// return; // can trigger only 1 interrupt at a time
	}

	if(m_vint_pending && _sett.vertical_interrupt_enabled())
	{
		if(m68k_int == nullptr)
			throw genesis::internal_error();
		// std::cout << "Raising VINT\n";
		m68k_int->interrupt_priority(6);
	}
}

void vdp::on_interrupt(std::uint8_t ipl)
{
	if(ipl == 6)
	{
		// std::cout << "ACK VINT\n";
		m_vint_pending = false;
	}

	if(ipl == 4)
	{
		std::cout << "ACK HINT\n";
		m_hint_pending = false;
		hint_counter = 0;
	}

	if(m_vint_pending)
	{
		// std::cout << "Raising VINT\n";
		m68k_int->interrupt_priority(6);
	}
	else if(m_hint_pending)
	{
		// std::cout << "Raising HINT\n";
		m68k_int->interrupt_priority(4);
	}
	else
		m68k_int->interrupt_priority(0);
}

void vdp::update_hv_counters()
{
	if(mclk % (cycles_per_pixel(_sett) * 2) == 0)
	{
		auto width = _sett.display_width();
		inc_h_counter();
		update_hblank(width);

		if((width == display_width::c32 && m_h_counter.raw_value() == 0x85) ||
			(width == display_width::c40 && m_h_counter.raw_value() == 0xA5))
		{
			inc_v_counter();
			update_vblank(_sett.display_height(), MODE);
		}

		int hint_threshold = _sett.display_width() == display_width::c32 ? 0x86 : 0xA6;
		if(hint_counter > 0 && regs.SR.VB == 0 && m_h_counter.raw_value() == hint_threshold)
		{
			--hint_counter;
			if(hint_counter == 0)
				m_hint_pending = true;
		}
	}
}

void vdp::inc_h_counter()
{
	m_h_counter.inc(_sett.display_width());
	regs.h_counter = m_h_counter.value();
}

void vdp::inc_v_counter()
{
	m_v_counter.inc(_sett.display_height(), MODE);
	regs.v_counter = m_v_counter.value();
}

void vdp::update_vblank(display_height height, genesis::vdp::mode mode)
{
	m_vblank_flag.update(m_v_counter.raw_value(), height, mode);

	// VB changes 0 -> 1
	if(m_vblank_flag.value() == true && regs.SR.VB == 0)
	{
		m_vint_pending = true;
	}

	regs.SR.VB = m_vblank_flag.value() ? 1 : 0;
}

void vdp::update_hblank(display_width width)
{
	m_hblank_flag.update(m_h_counter.raw_value(), width);
	regs.SR.HB = m_hblank_flag.value() ? 1 : 0;
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