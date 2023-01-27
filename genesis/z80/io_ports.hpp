#ifndef __IO_PORTS_HPP__
#define __IO_PORTS_HPP__

#include <cstdint>

namespace genesis::z80
{

// primitive implementation so far

class io_ports
{
public:
	virtual void out(std::uint8_t dev, std::uint8_t data) = 0;
};

}

#endif // __IO_PORTS_HPP__
