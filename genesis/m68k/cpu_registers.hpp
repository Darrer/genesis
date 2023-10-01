#ifndef __M68K_CPU_REGISTERS_HPP__
#define __M68K_CPU_REGISTERS_HPP__

#include "exception.hpp"
#include "impl/size_type.h"

#include <cstdint>

namespace genesis::m68k
{

/* m68k stores data in big endian, however, cpu_registers class stores data in system endian */

union data_register {
	std::uint8_t B;
	std::uint16_t W;
	std::uint32_t LW;
};

union address_register {
	std::uint16_t W;
	std::uint32_t LW;
};

struct status_register
{
	/* user byte */
	std::uint8_t C : 1;
	std::uint8_t V : 1;
	std::uint8_t Z : 1;
	std::uint8_t N : 1;
	std::uint8_t X : 1;

	std::uint8_t U1 : 1;
	std::uint8_t U2 : 1;
	std::uint8_t U3 : 1;

	/* system byte */
	std::uint8_t IPM : 3;
	std::uint8_t U4 : 1;
	std::uint8_t U5 : 1;
	std::uint8_t S : 1;
	std::uint8_t U6 : 1;
	std::uint8_t TR : 1;
};

static_assert(sizeof(data_register) == 4);
static_assert(sizeof(address_register) == 4);
static_assert(sizeof(status_register) == 2);


class cpu_registers
{
public:
	cpu_registers()
	{
		D0.LW = D1.LW = D2.LW = D3.LW = D4.LW = D5.LW = D6.LW = D7.LW = 0x0;
		A0.LW = A1.LW = A2.LW = A3.LW = A4.LW = A5.LW = A6.LW = 0x0;
		PC = SPC = 0x0;
		SR = 0x0;
		USP.LW = SSP.LW = 0x0;
		IRC = IR = IRD = SIRD = 0x0;
	}

	data_register& D(std::uint_fast8_t reg)
	{
		switch(reg)
		{
		case 0:
			return D0;
		case 1:
			return D1;
		case 2:
			return D2;
		case 3:
			return D3;
		case 4:
			return D4;
		case 5:
			return D5;
		case 6:
			return D6;
		case 7:
			return D7;
		default:
			throw internal_error();
		}
	}

	address_register& A(std::uint_fast8_t reg)
	{
		switch(reg)
		{
		case 0:
			return A0;
		case 1:
			return A1;
		case 2:
			return A2;
		case 3:
			return A3;
		case 4:
			return A4;
		case 5:
			return A5;
		case 6:
			return A6;
		case 7:
			return SP();
		default:
			throw internal_error();
		}
	}

	address_register& SP()
	{
		return flags.S ? SSP : USP;
	}

	void inc_addr(std::uint_fast8_t reg, size_type size)
	{
		if(reg == 0b111 && size == size_type::BYTE)
			size = size_type::WORD;
		A(reg).LW += size_in_bytes(size);
	}

	void dec_addr(std::uint_fast8_t reg, size_type size)
	{
		if(reg == 0b111 && size == size_type::BYTE)
			size = size_type::WORD;
		A(reg).LW -= size_in_bytes(size);
	}

	data_register D0, D1, D2, D3, D4, D5, D6, D7;
	address_register A0, A1, A2, A3, A4, A5, A6;
	address_register USP;
	address_register SSP;

	std::uint32_t PC;
	std::uint32_t SPC; // contains initial PC value of instruction being executed

	union {
		std::uint16_t SR;
		status_register flags;
	};

	/* Prefetch queue registers */
	std::uint_fast16_t IRC;
	std::uint_fast16_t IR;
	std::uint_fast16_t IRD;
	std::uint_fast16_t SIRD; // contains opcode of instruction being executed
};

} // namespace genesis::m68k

#endif // __M68K_CPU_REGISTERS_HPP__
