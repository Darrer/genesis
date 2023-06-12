#ifndef __VDP_MEMORY_H__
#define __VDP_MEMORY_H__

#include "memory.hpp"
#include <array>

namespace genesis::vdp
{

using vram_t = genesis::memory<std::uint16_t, 0xffff, std::endian::little>;

// 64 16-bit words
using cram_t = std::array<std::uint16_t, 64>;

// 40 16-bit words
using vsram_t = std::array<std::uint16_t, 40>;

};

#endif // __VDP_MEMORY_H__
