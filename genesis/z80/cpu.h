#ifndef __Z80_CPU_H__
#define __Z80_CPU_H__

#include "cpu_bus.hpp"
#include "cpu_registers.hpp"
#include "io_ports.hpp"
#include "memory.h"

#include <cstdint>
#include <memory>
#include <vector>


namespace genesis::z80
{

using opcode = std::uint8_t;

enum class cpu_interrupt_mode
{
	im0,
	im1,
	im2
};


class executioner;

class cpu
{
public:
	cpu(std::shared_ptr<z80::memory> memory, std::shared_ptr<z80::io_ports> io_ports = nullptr);
	~cpu();

	void execute_one();

	// TODO: do we need to make all these methods public?

	cpu_registers& registers()
	{
		return regs;
	}

	cpu_bus& bus()
	{
		return _bus;
	}

	z80::memory& memory()
	{
		return *mem;
	}

	z80::io_ports& io_ports()
	{
		// It's okay to have CPU without IO ports as most of the program do not use it,
		// so throw exception only when a program makes an attempt to execute IN/OUT commands
		if(ports == nullptr)
			throw std::runtime_error("Z80 cpu was not configured with io ports");
		return *ports;
	}

	cpu_interrupt_mode interrupt_mode() const
	{
		return int_mode;
	}

	void interrupt_mode(cpu_interrupt_mode mode)
	{
		int_mode = mode;
	}

	void reset();

private:
	std::unique_ptr<z80::executioner> exec;
	std::shared_ptr<z80::memory> mem;
	std::shared_ptr<z80::io_ports> ports;
	cpu_registers regs;
	cpu_bus _bus;
	cpu_interrupt_mode int_mode;
};

} // namespace genesis::z80


#endif // __Z80_CPU_H__
