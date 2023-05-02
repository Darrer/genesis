#ifndef __M68K_BASE_UNIT_H__
#define __M68K_BASE_UNIT_H__

#include <cstdint>

#include "m68k/cpu_registers.hpp"
#include "bus_scheduler.h"


namespace genesis::m68k
{

class base_unit
{
protected:
	enum class exec_state
	{
		done,
		wait_scheduler,
		in_progress,
	};

public:
	base_unit(m68k::cpu_registers& regs, m68k::bus_scheduler& scheduler);
	virtual ~base_unit() { }

	void cycle();
	void post_cycle();
	virtual bool is_idle() const;
	virtual void reset();

protected:
	// virtual void on_idle() = 0; // TODO
	virtual exec_state on_executing() = 0;

protected:
	/* interface for sub classes */

	void read(std::uint32_t addr, size_type size);
	void dec_and_read(std::uint8_t addr_reg, size_type size);
	void read_imm(size_type size);

private:
	void executing();

protected:
	m68k::cpu_registers& regs;
	m68k::bus_scheduler& scheduler;

	std::uint32_t imm;
	std::uint32_t data;

private:
	std::uint8_t state;
};

}

#endif //__M68K_BASE_UNIT_H__
