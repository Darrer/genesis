#ifndef __Z80_CPU_UNIT_HPP__
#define __Z80_CPU_UNIT_HPP__

// TODO: rename impl folder to units

namespace genesis::z80
{

class cpu::unit
{
public:
	virtual ~unit() = default;

	// TODO: rename to try_execute?
	virtual bool execute() = 0;

public:
	class base_unit;
	class arithmetic_logic_unit;
};

} // namespace genesis::z80


#endif // __Z80_CPU_UNIT_HPP__