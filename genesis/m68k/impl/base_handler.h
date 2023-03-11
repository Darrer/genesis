#ifndef __M68K_BASE_HANDLER_H__
#define __M68K_BASE_HANDLER_H__

#include <cstdint>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "m68k/cpu_registers.hpp"


namespace genesis::m68k
{

class base_handler
{
public:
	base_handler(m68k::cpu_registers& regs, m68k::bus_manager& busm, m68k::prefetch_queue& pq);

	void reset();
	bool is_idle() const;
	void cycle();

protected:
	/* interface for sub classes */

	void read_byte(std::uint32_t addr);
	void read_word(std::uint32_t addr);
	void read_long(std::uint32_t addr);

	void write_byte(std::uint32_t addr, std::uint8_t data);
	void write_word(std::uint32_t addr, std::uint16_t data);
	void write_long(std::uint32_t addr, std::uint32_t data);

	void write_byte_and_idle(std::uint32_t addr, std::uint8_t data);
	void write_word_and_idle(std::uint32_t addr, std::uint16_t data);
	void write_long_and_idle(std::uint32_t addr, std::uint32_t data);

	void prefetch();
	void prefetch_and_idle();

	void read_imm(std::uint8_t size);

protected:
	/* must be overridden by sub classes */

	virtual void on_cycle() = 0;
	virtual void on_idle() = 0;

protected:
	m68k::cpu_registers& regs;
	m68k::bus_manager& busm;
	m68k::prefetch_queue& pq;

	std::uint32_t imm;

private:
	std::uint8_t state;
};

}

#endif //__M68K_BASE_HANDLER_H__
