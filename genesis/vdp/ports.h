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

	std::uint16_t read_control() const
	{
		return regs.sr_raw;
	}

	void write_control(std::uint16_t data)
	{
		if((data >> 14) == 0b10)
		{
			// write data to register
			std::uint8_t reg_data = data & 0xFF;
			std::uint8_t reg_num = (data >> 8) & 0b11111;
			if(reg_num <= 23)
			{
				regs.set_register(reg_num, reg_data);
			}
		}
	}

private:
	register_set& regs;
};

}

#endif // __VDP_PORTS_H__