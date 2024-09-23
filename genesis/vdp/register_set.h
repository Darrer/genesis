#ifndef __VDP_REGISTER_SET_H__
#define __VDP_REGISTER_SET_H__

#include <cstring>

#include "control_register.h"
#include "exception.hpp"
#include "impl/fifo.h"
#include "read_buffer.h"
#include "registers.h"


namespace genesis::vdp
{

class register_set
{
public:
	register_set()
	{
		m_registers = {
			&R0, &R1, &R2, &R3, &R4, &R5, &R6, &R7, &R8, &R9, &R10,
			&R11, &R12, &R13, &R14, &R15, &R16, &R17, &R18, &R19, &R20,
			&R21, &R22, &R23
		};

		for(std::size_t i = 0; i < m_registers.size(); ++i)
			set_register((int)i, 0);
		h_counter = v_counter = 0;
		sr_raw = 0;
	}

	void set_register(int reg, std::uint8_t value)
	{
		std::memcpy(m_registers.at(reg), &value, sizeof(value));
	}

	std::uint8_t get_register(std::uint8_t reg)
	{
		std::uint8_t value;
		std::memcpy(&value, m_registers.at(reg), sizeof(value));
		return value;
	}

	/* Registers */

	vdp::R0 R0;
	vdp::R1 R1;
	vdp::R2 R2;
	vdp::R3 R3;
	vdp::R4 R4;
	vdp::R5 R5;
	vdp::R6 R6;
	vdp::R7 R7;
	vdp::R8 R8;
	vdp::R9 R9;
	vdp::R10 R10;
	vdp::R11 R11;
	vdp::R12 R12;
	vdp::R13 R13;
	vdp::R14 R14;
	vdp::R15 R15;
	vdp::R16 R16;
	vdp::R17 R17;
	vdp::R18 R18;
	vdp::R19 R19;
	vdp::R20 R20;
	vdp::R21 R21;
	vdp::R22 R22;
	vdp::R23 R23;

	union {
		status_register SR;
		std::uint16_t sr_raw;
	};

	std::uint8_t h_counter;
	std::uint8_t v_counter;

	control_register control;
	read_buffer read_cache;
	vdp::fifo fifo;

private:
	std::array<void*, 24> m_registers;
};

} // namespace genesis::vdp

#endif // __VDP_REGISTER_SET_H__