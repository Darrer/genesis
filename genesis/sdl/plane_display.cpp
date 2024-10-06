#include "plane_display.h"

namespace genesis::sdl
{

std::array<genesis::vdp::output_color, 1024> plane_display::buffer;
std::array<std::uint32_t, 1048576> plane_display::pixels;

} // namespace genesis::sdl
