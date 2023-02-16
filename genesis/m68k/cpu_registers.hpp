#ifndef __M68K_CPU_REGISTERS_HPP__
#define __M68K_CPU_REGISTERS_HPP__

#include <cstdint>


namespace genesis::m68k
{

/* m68k stores data in big endian, however, cpu_registers class stores data in system endian */

class cpu_registers
{
public:
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
		std::uint8_t M : 1;
		std::uint8_t S : 1;
		std::uint8_t TR : 2;
	};

	static_assert(sizeof(status_register) == 2);

public:
	cpu_registers() : USP(A7)
	{
		D0.LW = D1.LW = D2.LW = D3.LW = D4.LW = D5.LW = D6.LW = D7.LW = 0x0;
		A0.LW = A1.LW = A2.LW = A3.LW = A4.LW = A5.LW = A6.LW = A7.LW = 0x0;
		PC = 0x0;

		SR.C = SR.V = SR.Z = SR.N = SR.X = SR.U1 = SR.U2 = SR.U3 = 0x0;
		SR.IPM = SR.U4 = SR.M = SR.S = SR.TR = 0x0;
	}

	data_register D0, D1, D2, D3, D4, D5, D6, D7;

	address_register A0, A1, A2, A3, A4, A5, A6, A7;
	address_register& USP;

	std::uint32_t PC;
	status_register SR;
};

} // namespace genesis::m68k

#endif // __M68K_CPU_REGISTERS_HPP__
