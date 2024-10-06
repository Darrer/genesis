#ifndef __IO_PORTS_INPUT_DEVICE_H__
#define __IO_PORTS_INPUT_DEVICE_H__

#include "key_type.h"

namespace genesis::io_ports
{

class input_device
{
public:
	virtual ~input_device() = default;

	virtual bool is_key_pressed(key_type) = 0;
};

} // namespace genesis::io_ports

#endif // __IO_PORTS_INPUT_DEVICE_H__
