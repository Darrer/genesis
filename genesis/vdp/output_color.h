#ifndef __VDP_OUTPUT_COLOR_H__
#define __VDP_OUTPUT_COLOR_H__

#include <cstdint>

#include "impl/color.h"

namespace genesis::vdp
{

// color used for external interface
struct output_color
{
	output_color(std::uint16_t col) : output_color(impl::color(col)) { }
	output_color(impl::color col) : red(col.red), green(col.green), blue(col.blue), transparent(false) { }
	output_color() : red(7), green(7), blue(7), transparent(true) { }

	bool operator==(const output_color& rcolor) const = default;

	std::uint8_t red = 0;
	std::uint8_t green = 0;
	std::uint8_t blue = 0;
	bool transparent = false;
};

static const output_color TRANSPARENT_COLOR;


}

#endif // __VDP_OUTPUT_COLOR_H__
