#include "ports.h"

namespace genesis::vdp
{

void ports::init_read_control()
{
	start(request::read_control);
}

void ports::init_write_control(std::uint8_t data)
{
	std::uint16_t ex_data = std::uint16_t(data) << 8;
	ex_data |= data;
	init_write_control(ex_data);
}

void ports::init_write_control(std::uint16_t data)
{
	start(request::write_control);
	data_to_write = data;
}

void ports::init_read_data()
{
	start(request::read_data);
}

void ports::init_write_data(std::uint8_t data)
{
	std::uint16_t ex_data = std::uint16_t(data) << 8;
	ex_data |= data;
	init_write_data(ex_data);
}

void ports::init_write_data(std::uint16_t data)
{
	start(request::write_data);
	data_to_write = data;
}

std::uint16_t ports::read_result()
{
	if(reading_control_port)
	{
		// always return current SR value
		return regs.sr_raw;
	}

	if(read_data.has_value())
	{
		return read_data.value();
	}

	throw internal_error("ports::read_result: no data available");
}

bool ports::is_idle() const
{
	return req == request::none;
}

void ports::cycle()
{
	// note: so far assume it takes only 1 cycle to execute the request
	// if it should take 0 - move handler to init_* method
	// if more - add wait cycles

	switch(req)
	{
	case request::none:
		break;

	case request::read_control:
		req = request::none;
		reading_control_port = true;
		control_pending = false;
		break;

	case request::write_control:
		if(_control_write_request.has_value())
		{
			// wait till vdp picks up the request
			break;
		}

		if(control_pending == false && (data_to_write >> 14) == 0b10)
		{
			// write data to register
			_control_write_request = {data_to_write, true};
		}
		else
		{
			if(!control_pending)
			{
				// first half
				control_pending = true;
				_control_write_request = {data_to_write, true};
			}
			else
			{
				// second half
				control_pending = false;
				_control_write_request = {data_to_write, false};
			}
		}

		// we're done
		req = request::none;

		break;

	case request::read_data:
		if(regs.control.work_completed())
		{
			read_data = regs.read_cache.data();

			// read is over, inc address
			regs.control.address(regs.control.address() + regs.R15.INC);

			// remove the complete flag so vdp can pre-cache next read
			regs.control.work_completed(false);

			// reading from data port must clear the pending flag
			control_pending = false;

			req = request::none;
		}

		break;

	case request::write_data:
		// note: write directly to queue
		if(regs.fifo.full())
		{
			// wait till fifo get a free slot
			break;
		}

		regs.fifo.push(data_to_write, regs.control);

		// write is over, inc address
		regs.control.address(regs.control.address() + regs.R15.INC);

		// writing to data port must clear the pending flag
		control_pending = false;

		req = request::none;

		break;

	default:
		throw internal_error();
	}
}

void ports::reset()
{
	start(request::none);
}

void ports::start(request _req)
{
	req = _req;
	reading_control_port = false;
	data_to_write = 0;
	read_data.reset();
}

}; // namespace genesis::vdp
