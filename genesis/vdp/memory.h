#ifndef __VDP_MEMORY_H__
#define __VDP_MEMORY_H__

#include <array>
#include "memory/memory_unit.h"


namespace genesis::vdp
{

class vram_t : public memory::memory_unit
{
public:
	vram_t() : memory::memory_unit(0xffff, std::endian::big) // [0; 0xFFFF]
	{
	}
};

class cram_t
{
public:
	cram_t() : mem(127) // 128 bytes [0 ; 127]
	{
	}

	std::uint16_t read(std::uint16_t addr)
	{
		return mem.read<std::uint16_t>(format_addr(addr));
	}

	void write(std::uint16_t addr, std::uint16_t data)
	{
		mem.write(format_addr(addr), data);
	}

	std::uint16_t read_color(unsigned palette, unsigned color_idx)
	{
		if(palette > 4)
			throw std::invalid_argument("palette");
		if(color_idx > 16)
			throw std::invalid_argument("color_idx");

		std::uint16_t addr = (palette * 32) + (color_idx * 2);
		return read(addr);
	}

private:
	static std::uint16_t format_addr(std::uint16_t addr)
	{
		return addr & 0b0000000001111110;
	}

private:
	memory::memory_unit mem;
};


class vsram_t
{
public:
	vsram_t() : mem(79) // 80 bytes [0 ; 79]
	{
	}

	std::uint16_t read(std::uint16_t addr)
	{
		addr = format_addr(addr);

		if(addr >= 80)
		{
			// TODO: in such case we have to return current VSRAM latched value
			// and this logic should be handled by VDP core, but for simplicity
			// return dummy value now
			return 0;
		}

		return mem.read<std::uint16_t>(format_addr(addr));
	}

	void write(std::uint16_t addr, std::uint16_t data)
	{
		addr = format_addr(addr);
		if(addr >= 80)
			return;

		mem.write(addr, data);
	}

private:
	static std::uint16_t format_addr(std::uint16_t addr)
	{
		return addr & 0b0000000001111110;
	}

private:
	memory::memory_unit mem;
};

}; // namespace genesis::vdp

#endif // __VDP_MEMORY_H__
