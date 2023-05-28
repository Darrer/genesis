#ifndef __VDP_REGISTER_SET_H__
#define __VDP_REGISTER_SET_H__

#include "registers.h"
#include "exception.hpp"


namespace genesis::vdp
{

class register_set
{
public:
	register_set()
	{
		for(std::uint8_t i = 0; i <= 23; ++i)
			set_register(i, 0);
	}

	void set_register(std::uint8_t reg, std::uint8_t value)
	{
		get_register(reg) = value;
	}

	std::uint8_t& get_register(std::uint8_t reg)
	{
		switch (reg)
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

		default: throw internal_error();
		}
	}

	R0 R0;
	R1 R1;
	R2 R2;
	R3 R3;
	R4 R4;
	R5 R5;
	R6 R6;
	R7 R7;
	R8 R8;
	R9 R9;
	R10 R10;
	R11 R11;
	R12 R12;
	R13 R13;
	R14 R14;
	R15 R15;
	R16 R16;
	R17 R17;
	R18 R18;
	R19 R19;
	R20 R20;
	R21 R21;
	R22 R22;
	R23 R23;

	union
	{
		status_register SR;
		std::uint16_t sr_raw;
	};
};

}

#endif // __VDP_REGISTER_SET_H__