#ifndef __OPERATIONS_HPP__
#define __OPERATIONS_HPP__

#include "../cpu.h"
#include <cstdint>

namespace genesis::z80
{

class operations
{
public:
	/* 8-Bit Arithmetic Group */
	inline static void add(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = ((_a & 0x0f) + (_b & 0x0f) > 0xf) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a + (int)_b > 127 || (int)_a + (int)_b < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = (unsigned)_a + (unsigned)_b > 0xff ? 1 : 0;

		flags.N = 0;

		regs.main_set.A = _a + _b;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void adc(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = ((_a & 0x0f) + (_b & 0x0f) + _c > 0xf) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a + (int)_b + _c > 127 || (int)_a + (int)_b + _c < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = (unsigned)_a + (unsigned)_b + (unsigned)_c > 0xff ? 1 : 0;

		flags.N = 0;

		regs.main_set.A = _a + _b + _c;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void sub(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = (_b & 0x0f) > (_a & 0x0f) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a - (int)_b > 127 || (int)_a - (int)_b < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = _b > _a  ? 1 : 0;

		flags.N = 1;

		regs.main_set.A = _a - _b;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void sbc(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;
		std::uint8_t _c = regs.main_set.flags.C;

		auto& flags = regs.main_set.flags;

		// check Half Carry Flag (H)
		flags.H = (_b & 0x0f) + _c > (_a & 0x0f) ? 1 : 0;

		// check Parity/Overflow Flag (P/V)
		flags.PV = (int)_a - (int)_b - _c > 127 || (int)_a - (int)_b - _c < -128 ? 1 : 0;

		// check Carry Flag (C)
		flags.C = _b + _c > _a  ? 1 : 0;

		flags.N = 1;

		regs.main_set.A = _a - _b - _c;

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void and_8(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a & _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void or_8(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a | _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}

	inline static void xor_8(z80::cpu_registers& regs, std::int8_t b)
	{
		std::uint8_t _a = (std::uint8_t)regs.main_set.A;
		std::uint8_t _b = (std::uint8_t)b;

		auto& flags = regs.main_set.flags;

		flags.H = flags.N = flags.C = 0;

		regs.main_set.A = _a ^ _b;

		// check Parity/Overflow Flag (P/V)
		flags.PV = check_parity(regs.main_set.A);

		flags.S = (regs.main_set.A < 0) ? 1 : 0;
		flags.Z = (regs.main_set.A == 0) ? 1 : 0;
	}


	/* utils */
	static std::uint8_t check_parity(int n) 
	{
		int b;
		b = n ^ (n >> 1); 
		b = b ^ (b >> 2); 
		b = b ^ (b >> 4); 
		b = b ^ (b >> 8); 
		b = b ^ (b >> 16); 
		if (b & 1)
			return 1;
		else
			return 0;
	}
};

} // genesis namespace

#endif // __OPERATIONS_HPP__
