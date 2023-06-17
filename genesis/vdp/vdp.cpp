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
				regs.address.set_a1(data);
			else
				regs.address.set_a2(data);
		}

		write_req.reset();
	}
}

} // namespace genesis::vdp