#ifndef __Z80_BASE_CPU_UNIT_HPP__
#define __Z80_BASE_CPU_UNIT_HPP__

#include "../cpu.h"
#include "cpu_unit.hpp"

#include <initializer_list>


namespace genesis::z80
{

class cpu::unit::base_unit : public cpu::unit
{
public:
	base_unit(z80::cpu& cpu) : cpu(cpu)
	{
	}

protected:
	bool is_next_opcode(std::initializer_list<z80::opcode> op)
	{
		if (op.size() == 0)
			throw std::runtime_error("is_next_opcode: internal error, opcode cannot be empty");

		auto& mem = cpu.memory();

		size_t offset = cpu.registers().PC;
		for (auto& _op : op)
		{
			auto mem_opcode = mem.read<z80::opcode>(offset++);
			if (mem_opcode != _op)
				return false;
		}

		return true;
	}

	bool is_next_opcode(z80::opcode op)
	{
		return is_next_opcode({op});
	}

	template<class T>
	inline T fetch(z80::memory::address addr)
	{
		return cpu.memory().read<T>(addr);
	}

	template<class T = z80::opcode>
	inline T fetch_pc(z80::memory::address offset = 0)
	{
		return fetch<T>(cpu.registers().PC + offset);
	}

	template<class T>
	inline T fetch_hl(z80::memory::address offset = 0)
	{
		return fetch<T>(cpu.registers().main_set.HL + offset);
	}
protected:
	z80::cpu& cpu;
};


} // namespace genesis::z80

#endif // __Z80_BASE_CPU_UNIT_HPP__
