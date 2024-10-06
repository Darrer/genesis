#ifndef __IO_PORTS_DISABLED_PORT_H__
#define __IO_PORTS_DISABLED_PORT_H__

#include "memory/addressable.h"
#include "memory/dummy_memory.h"

#include <memory>

namespace genesis::io_ports
{

class disabled_port
{
public:
	disabled_port() = delete;

	static std::unique_ptr<memory::addressable> data()
	{
		return std::make_unique<memory::ffff_memory_unit>(0x1, std::endian::big);
	}

	static std::unique_ptr<memory::addressable> control()
	{
		return std::make_unique<memory::zero_memory_unit>(0x1, std::endian::big);
	}
};

} // namespace genesis::io_ports

#endif // __IO_PORTS_DISABLED_PORT_H__
