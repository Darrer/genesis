#ifndef __VDP_REGISTER_SET_H__
#define __VDP_REGISTER_SET_H__

#include "control_register.h"
#include "exception.hpp"
#include "impl/fifio.h"
#include "read_buffer.h"
#include "registers.h"


namespace genesis::vdp
{

class register_set
{
public:
	register_set()
	{
		for(std::uint8_t i = 0; i <= 23; ++i)
			set_register(i, 0);
		sr_raw = 0;
	}

	void set_register(std::uint8_t reg, std::uint8_t value)
	{
		get_register(reg) = value;
	}

	std::uint8_t& get_register(std::uint8_t reg)
	{
		// TODO: check if it's UB to use reinterpret_cast here
		switch(reg)
		{
		case 0:
			return *reinterpret_cast<std::uint8_t*>(&R0);
		case 1:
			return *reinterpret_cast<std::uint8_t*>(&R1);
		case 2:
			return *reinterpret_cast<std::uint8_t*>(&R2);
		case 3:
			return *reinterpret_cast<std::uint8_t*>(&R3);
		case 4:
			return *reinterpret_cast<std::uint8_t*>(&R4);
		case 5:
			return *reinterpret_cast<std::uint8_t*>(&R5);
		case 6:
			return *reinterpret_cast<std::uint8_t*>(&R6);
		case 7:
			return *reinterpret_cast<std::uint8_t*>(&R7);
		case 8:
			return *reinterpret_cast<std::uint8_t*>(&R8);
		case 9:
			return *reinterpret_cast<std::uint8_t*>(&R9);
		case 10:
			return *reinterpret_cast<std::uint8_t*>(&R10);
		case 11:
			return *reinterpret_cast<std::uint8_t*>(&R11);
		case 12:
			return *reinterpret_cast<std::uint8_t*>(&R12);
		case 13:
			return *reinterpret_cast<std::uint8_t*>(&R13);
		case 14:
			return *reinterpret_cast<std::uint8_t*>(&R14);
		case 15:
			return *reinterpret_cast<std::uint8_t*>(&R15);
		case 16:
			return *reinterpret_cast<std::uint8_t*>(&R16);
		case 17:
			return *reinterpret_cast<std::uint8_t*>(&R17);
		case 18:
			return *reinterpret_cast<std::uint8_t*>(&R18);
		case 19:
			return *reinterpret_cast<std::uint8_t*>(&R19);
		case 20:
			return *reinterpret_cast<std::uint8_t*>(&R20);
		case 21:
			return *reinterpret_cast<std::uint8_t*>(&R21);
		case 22:
			return *reinterpret_cast<std::uint8_t*>(&R22);
		case 23:
			return *reinterpret_cast<std::uint8_t*>(&R23);

		default:
			throw internal_error();
		}
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

	control_register control;
	read_buffer read_cache;
	vdp::fifo fifo;
};

} // namespace genesis::vdp

#endif // __VDP_REGISTER_SET_H__