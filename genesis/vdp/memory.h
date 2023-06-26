#ifndef __VDP_MEMORY_H__
#define __VDP_MEMORY_H__

#include "memory.hpp"
#include <array>

namespace genesis::vdp
{

using vram_t = genesis::memory<std::uint16_t, 0xffff, std::endian::little>;

class cram_t
{
public:
	std::uint16_t read(std::uint16_t addr)
	{
		return mem.read<std::uint16_t>(format_addr(addr));
	}

	void write(std::uint16_t addr, std::uint16_t data)
	{
		mem.write(format_addr(addr), data);
	}

private:
	static std::uint16_t format_addr(std::uint16_t addr)
	{
		return addr & 0b0000000001111110;
	}

private:
	// TODO: do not speicfy endianess
	// 128 bytes [0 ; 127]
	genesis::memory<std::uint16_t, 127, std::endian::little> mem;
};


class vsram_t
{
public:
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
	// TODO: do not speicfy endianess
	// 80 bytes [0 ; 79]
	genesis::memory<std::uint16_t, 79, std::endian::little> mem;
};

};

#endif // __VDP_MEMORY_H__
