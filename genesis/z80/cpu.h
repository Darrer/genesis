#ifndef __Z80_CPU_H__
#define __Z80_CPU_H__

#include "cpu_registers.hpp"
#include "io_ports.hpp"
#include "memory.hpp"

#include <cstdint>
#include <memory>
#include <vector>


namespace genesis::z80
{

using memory = genesis::memory<std::uint16_t, 0xffff, std::endian::little>;
using opcode = std::uint8_t;

class executioner;

class cpu
{
public:
	cpu(std::shared_ptr<z80::memory> memory, std::shared_ptr<z80::io_ports> io_ports = nullptr);
	~cpu();

	inline cpu_registers& registers()
	{
		return regs;
	}

	inline memory& memory()
	{
		return *mem;
	}

	inline io_ports& io_ports()
	{
		return *ports;
	}

	void execute_one();

private:
	std::unique_ptr<z80::executioner> exec;
	std::shared_ptr<z80::memory> mem;
	std::shared_ptr<z80::io_ports> ports;
	cpu_registers regs;
};

} // namespace genesis::z80


#endif // __Z80_CPU_H__
