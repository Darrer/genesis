#ifndef __VDP_OUTPUT_COLOR_H__
#define __VDP_OUTPUT_COLOR_H__

#include <cstdint>

namespace genesis::vdp
{

// color used for external interface
// TODO: rename to pixel?
struct output_color
{
	output_color(std::uint16_t value)
	{
		// Internal color representation:
		// ----bbb-ggg-rrr-
		red = (value >> 1) & 0b111;
		green = (value >> 5) & 0b111;
		blue = (value >> 9) & 0b111;

		transparent = false;
	}

	output_color() : red(0), green(0), blue(0), transparent(true) { }

	bool operator==(const output_color& rcolor) const = default;

	// Convert color back to internal representation
	std::uint16_t to_internal() const
	{
		std::uint16_t val = 0;
		val |= red << 1;
		val |= green << 5;
		val |= blue << 9;
		return val;
	}

	std::uint8_t red : 3;
	std::uint8_t green : 3;
	std::uint8_t blue : 3;
	bool transparent : 1;
};

static_assert(sizeof(output_color) == 2);

static const output_color TRANSPARENT_COLOR;


}

#endif // __VDP_OUTPUT_COLOR_H__
