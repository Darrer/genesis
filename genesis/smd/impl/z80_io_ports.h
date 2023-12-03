#ifndef __SMD_IMPL_Z80_IO_PORTS_H__
#define __SMD_IMPL_Z80_IO_PORTS_H__

#include "z80/io_ports.hpp"

namespace genesis::impl
{

class z80_io_ports : public z80::io_ports
{
public:
	std::uint8_t in(std::uint8_t /* dev */, std::uint8_t /* param */) override
	{
		// Do nothing
		return data++;
	}

	void out(std::uint8_t /* dev */, std::uint8_t /* param */, std::uint8_t /* data */) override
	{
		// Do nothing
	}

private:
	std::uint8_t data = 0x0;
};

}

#endif // __SMD_IMPL_Z80_IO_PORTS_H__
