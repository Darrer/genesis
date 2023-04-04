#ifndef __M68K_BASE_UNIT_H__
#define __M68K_BASE_UNIT_H__

#include <cstdint>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "bus_scheduler.h"
#include "m68k/cpu_registers.hpp"


namespace genesis::m68k
{

class base_unit
{
protected:
	enum class handler : std::uint8_t
	{
		in_progress,
		// done,
		wait_scheduler,
		wait_scheduler_and_done,
	};

public:
	base_unit(m68k::cpu_registers& regs, m68k::bus_manager& busm,
		m68k::prefetch_queue& pq, m68k::bus_scheduler& scheduler);
	virtual ~base_unit() { }

	void cycle();
	void post_cycle();
	bool is_idle() const;
	virtual void reset();

protected:
	virtual handler on_handler() = 0;

protected:
	/* interface for sub classes */

	void idle();

	void read(std::uint32_t addr, std::uint8_t size, bus_manager::on_complete cb = nullptr);
	void dec_and_read(std::uint8_t addr_reg, std::uint8_t size, bus_manager::on_complete cb = nullptr);
	void read_byte(std::uint32_t addr, bus_manager::on_complete cb = nullptr);
	void read_word(std::uint32_t addr, bus_manager::on_complete cb = nullptr);
	void read_long(std::uint32_t addr, bus_manager::on_complete cb = nullptr);

	void read_imm(std::uint8_t size, bus_manager::on_complete cb = nullptr);

	void wait_scheduler();
	void wait_scheduler_and_idle();

protected:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue& pq;
	m68k::bus_scheduler& scheduler;

	std::uint32_t imm;
	std::uint32_t data;

private:
	bus_manager::on_complete cb = nullptr;
	std::uint8_t state;
	std::uint8_t reg_to_dec = 0;
	bool go_idle;
};

}

#endif //__M68K_BASE_UNIT_H__
