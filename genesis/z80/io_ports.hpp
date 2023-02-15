#ifndef __IO_PORTS_HPP__
#define __IO_PORTS_HPP__

#include <cstdint>

namespace genesis::z80
{

class io_ports
{
public:
	virtual std::uint8_t in(std::uint8_t dev, std::uint8_t param) = 0;
	virtual void out(std::uint8_t dev, std::uint8_t param, std::uint8_t data) = 0;
};

} // namespace genesis::z80

#endif // __IO_PORTS_HPP__
