#ifndef __Z80_CPU_REGISTERS_HPP__
#define __Z80_CPU_REGISTERS_HPP__

#include <bit>
#include <cstdint>


namespace genesis::z80
{

/* z80 stores data in little endian */

// rely on IA-32 little endianness
static_assert(std::endian::native == std::endian::little);

struct flags
{
	std::uint8_t C : 1;
	std::uint8_t N : 1;
	std::uint8_t PV : 1;
	std::uint8_t X1 : 1;
	std::uint8_t H : 1;
	std::uint8_t X2 : 1;
	std::uint8_t Z : 1;
	std::uint8_t S : 1;
};

static_assert(sizeof(flags) == 1);


class register_set
{
private:
	union {
		struct
		{
			union {
				flags flags;
				std::int8_t f;
			};
			std::int8_t a;
		} af_t;
		std::int16_t af;
	};

	union {
		struct
		{
			std::int8_t c;
			std::int8_t b;
		} bc_t;
		std::int16_t bc;
	};

	union {
		struct
		{
			std::int8_t e;
			std::int8_t d;
		} de_t;
		std::int16_t de;
	};

	union {
		struct
		{
			std::int8_t l;
			std::int8_t h;
		} hl_t;
		std::int16_t hl;
	};

public:
	register_set()
		: A(af_t.a), F(af_t.f), AF(af), // 1st group
		  B(bc_t.b), C(bc_t.c), BC(bc), // 2nd group
		  D(de_t.d), E(de_t.e), DE(de), // 3rd group
		  H(hl_t.h), L(hl_t.l), HL(hl), // 4th group
		  flags(af_t.flags)
	{
		AF = BC = DE = HL = 0x0;
	}

	std::int8_t& A;
	std::int8_t& F;
	std::int16_t& AF;

	std::int8_t& B;
	std::int8_t& C;
	std::int16_t& BC;

	std::int8_t& D;
	std::int8_t& E;
	std::int16_t& DE;

	std::int8_t& H;
	std::int8_t& L;
	std::int16_t& HL;

	flags& flags;
};


class cpu_registers
{
public:
	cpu_registers()
	{
		I = R = 0x0;
		IX = IY = SP = PC = 0x0;
	}

	/* general purpose & accumulator/flag registers */

	register_set main_set;
	register_set alt_set;

	/* special purpose registers */

	std::int8_t I;
	std::int8_t R;

	std::uint16_t IX;
	std::uint16_t IY;

	std::uint16_t SP;
	std::uint16_t PC;
};

} // namespace genesis::z80

#endif // __Z80_CPU_REGISTERS_HPP__
