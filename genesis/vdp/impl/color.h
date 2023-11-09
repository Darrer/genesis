#ifndef __VDP_COLOR_H__
#define __VDP_COLOR_H__

#include  <cstdint>


namespace genesis::vdp::impl
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
		// format:
		// ----bbb-ggg-rrr-
		red = (_value >> 1) & 0b111;
		green = (_value >> 5) & 0b111;
		blue = (_value >> 9) & 0b111;
	}

	std::uint16_t value() const
	{
		std::uint16_t val = 0;
		val |= red << 1;
		val |= green << 5;
		val |= blue << 9;
		return val;
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
