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
	output_color() : red(0), green(0), blue(0), transparent(true) { }

	bool operator==(const output_color& rcolor) const = default;

	std::uint8_t red;
	std::uint8_t green;
	std::uint8_t blue;
	bool transparent = false;
};

static const output_color TRANSPARENT_COLOR;


}

#endif // __VDP_OUTPUT_COLOR_H__
