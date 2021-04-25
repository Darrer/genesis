#ifndef __Z80_CPU_REGISTERS_HPP__
#define __Z80_CPU_REGISTERS_HPP__

#include <bit>
#include <cstdint>


namespace genesis::z80
{

/* z80 stores data in little endian */

// relaying on IA-32 little endianness
static_assert(std::endian::native == std::endian::little);

class register_set
{
private:
	union {
		struct
		{
			std::uint8_t f;
			std::uint8_t a;
		} af_t;
		std::uint16_t af;
	};

	union {
		struct
		{
			std::uint8_t c;
			std::uint8_t b;
		} bc_t;
		std::uint16_t bc;
	};

	union {
		struct
		{
			std::uint8_t e;
			std::uint8_t d;
		} de_t;
		std::uint16_t de;
	};

	union {
		struct
		{
			std::uint8_t l;
			std::uint8_t h;
		} hl_t;
		std::uint16_t hl;
	};

public:
	register_set()
		: A(af_t.a), F(af_t.f), AF(af), // 1st group
		  B(bc_t.b), C(bc_t.c), BC(bc), // 2nd group
		  D(de_t.d), E(de_t.e), DE(de), // 3rd group
		  H(hl_t.h), L(hl_t.l), HL(hl)	// 4th group
	{
		AF = BC = DE = HL = 0x0;
	}

	std::uint8_t& A;
	std::uint8_t& F;
	std::uint16_t& AF;

	std::uint8_t& B;
	std::uint8_t& C;
	std::uint16_t& BC;

	std::uint8_t& D;
	std::uint8_t& E;
	std::uint16_t& DE;

	std::uint8_t& H;
	std::uint8_t& L;
	std::uint16_t& HL;
};


class cpu_registers
{
public:
	cpu_registers()
	{
		I = R = 0x0;
		IX = IY = SP = PC = 0x0;
	}

	/* general purpose & accumulator/flag registers*/

	register_set main_set;
	register_set alt_set;

	/* special purpose registers */

	std::uint8_t I;
	std::uint8_t R;

	std::uint16_t IX;
	std::uint16_t IY;

	std::uint16_t SP;
	std::uint16_t PC;
};

} // namespace genesis::z80

#endif // __Z80_CPU_REGISTERS_HPP__
