#ifndef __VDP_PORTS_H__
#define __VDP_PORTS_H__

#include "settings.h"
#include "register_set.h"


namespace genesis::vdp
{

class ports
{
public:
	ports(register_set& regs) : regs(regs) { }

	std::uint16_t read_control()
	{
		control_pending = false;
		return regs.sr_raw;
	}

	void write_control(std::uint8_t data)
	{
		std::uint16_t ex_data = std::uint16_t(data) << 8;
		ex_data |= data;
		write_control(ex_data);
	}

	void write_control(std::uint16_t data)
	{
		if(control_pending == false && (data >> 14) == 0b10)
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
			if(!control_pending)
			{
				// first half
				control_pending = true;
				regs.CP1_raw = data;
			}
			else
			{
				// second half
				control_pending = false;
				regs.CP2_raw = data;
			}
		}
	}

private:
	register_set& regs;

	// true if we're waiting for the 2nd control word
	bool control_pending = false;
};

}

#endif // __VDP_PORTS_H__