#ifndef __Z80_CPU_H__
#define __Z80_CPU_H__

#include "cpu_registers.hpp"
#include "memory.hpp"

#include <cstdint>
#include <memory>
#include <vector>


namespace genesis::z80
{

using z80_mem = genesis::memory<std::uint16_t, 0xffff, std::endian::little>;
using opcode = std::uint8_t;

class cpu
{
public:
	cpu(std::shared_ptr<z80_mem> memory);

	inline cpu_registers& registers()
	{
		return regs;
	}

	inline z80_mem& memory()
	{
		return *mem;
	}

	void execute_one();

private:
	bool try_execute();

private:
	std::shared_ptr<z80_mem> mem;
	cpu_registers regs;
};

} // namespace genesis::z80


#endif // __Z80_CPU_H__
