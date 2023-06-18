#ifndef __VDP_READ_BUFFER_H__
#define __VDP_READ_BUFFER_H__

#include <cstdint>

#include "exception.hpp"


namespace genesis::vdp
{

class read_buffer
{
public:
	read_buffer() { reset(); }

	std::uint16_t data() const
	{
		if(has_data())
			return _data;

		throw internal_error();
	}

	void set(std::uint16_t value)
	{
		_data = value;
		_has_data = true;
	}

	void set_lsb(std::uint8_t lsb)
	{
		_data &= 0xFF00;
		_data |= lsb;
	}

	void set_msb(std::uint8_t msb)
	{
		_data &= 0x00FF;
		_data |= std::uint16_t(msb) << 8;

		// this flag is set only when MSB is set
		_has_data = true;
	}

	bool has_data() const { return _has_data; }

	// keep the data, but remove the available flag
	void clear_data_flag()
	{
		_has_data = false;
	}

	// reset the data and the available flag
	void reset()
	{
		_data = 0;
		_has_data = false;
	}

private:
	// NOTE: assume little endian architecture!
	std::uint16_t _data;
	bool _has_data;
};

}

#endif // __VDP_READ_BUFFER_H__
