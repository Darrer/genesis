#ifndef __VDP_MEMORY_ACCESS_H__
#define __VDP_MEMORY_ACCESS_H__

#include "vdp/control_register.h"


namespace genesis::vdp::impl
{

// stores read/write requests to memory
// TODO: tmp interface
class memory_access
{
public:
	struct pending_write
	{
		vmem_type type;
		std::uint32_t address;
		std::uint16_t data;
	};

	struct pending_read
	{
		std::uint32_t address;
	};

public:
	template <class T>
	void init_write(vmem_type type, std::uint32_t address, T data)
	{
		static_assert(sizeof(T) <= 2);
		if(!is_idle())
			throw internal_error();

		write_req = {type, address, std::uint16_t(data)};
	}

	void init_read_vram(std::uint32_t address)
	{
		if(!is_idle())
			throw internal_error();

		struct pending_read read{address};
		read_req = read;
	}

	std::uint8_t latched_byte() const
	{
		return _read_data.value();
	}

	bool is_idle() const
	{
		return write_req.has_value() == false && read_req.has_value() == false;
	}

	// TODO: refactor read/write interface
	std::optional<pending_write>& pending_write()
	{
		return write_req;
	}
	std::optional<pending_read>& pending_read()
	{
		return read_req;
	}
	void set_read_result(std::uint8_t data)
	{
		_read_data = data;
	}

private:
	std::optional<struct pending_write> write_req;
	std::optional<struct pending_read> read_req;
	std::optional<std::uint8_t> _read_data;
};

} // namespace genesis::vdp::impl

#endif // __VDP_MEMORY_ACCESS_H__
