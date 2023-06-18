#ifndef __VDP_COLOR_H__
#define __VDP_COLOR_H__

#include  <cstdint>


namespace genesis::vdp
{

struct color
{
	color() = default;

	color(std::uint16_t _value)
	{
		value(_value);
	}

	void value(std::uint16_t _value)
	{
		_value = _value & 0b0111011101110000;
		*reinterpret_cast<std::uint16_t*>(this) = _value;
	}

	std::uint16_t value()
	{
		return *reinterpret_cast<std::uint16_t*>(this);
	}

	std::uint8_t : 1;
	std::uint8_t red : 3;

	std::uint8_t : 1;
	std::uint8_t green : 3;

	std::uint8_t : 1;
	std::uint8_t blue : 3;
	
	std::uint8_t : 4;
};

static_assert(sizeof(color) == 2);

}

#endif // __VDP_COLOR_H__
