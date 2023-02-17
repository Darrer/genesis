#ifndef __M68K_CPU_H__
#define __M68K_CPU_H__

#include "cpu_registers.hpp"
#include "memory.hpp"

#include <cstdint>
#include <memory>


namespace genesis::m68k
{

using memory = genesis::memory<std::uint32_t, 0x1000000 /* 16 MiB */, std::endian::big>;

class cpu
{
public:
	cpu(std::shared_ptr<m68k::memory> memory);

	cpu_registers& registers()
	{
		return regs;
	}

	memory& memory()
	{
		return *mem;
	}

private:
	std::shared_ptr<m68k::memory> mem;
	cpu_registers regs;
};

}

#endif // __M68K_CPU_H__
