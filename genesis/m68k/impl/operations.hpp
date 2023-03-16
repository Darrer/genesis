#ifndef __M68K_OPERATIONS_HPP__
#define __M68K_OPERATIONS_HPP__

#include <limits>

#include "m68k/cpu_registers.hpp"
#include "ea_decoder.hpp"

#include "exception.hpp"


namespace genesis::m68k
{

class operations
{
public:
	operations() = delete;

	static std::uint32_t add(data_register& reg, operand& op, std::uint8_t size, status_register& sr)
	{
		return add(value(reg, size), value(op, size), size, sr);
	}

	static std::uint32_t add(std::uint32_t val, operand& op, std::uint8_t size, status_register& sr)
	{
		return add(val, value(op, size), size, sr);
	}

private:
	static std::uint32_t add(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res;

		if(size == 1)
		{
			res = std::uint8_t(a + b);
			sr.V = check_overflow_add<std::int8_t>(a, b);
			sr.C = check_carry<std::uint8_t>(a, b);
			sr.N = (std::int8_t)res < 0;
		}
		else if(size == 2)
		{
			res = std::uint16_t(a + b);
			sr.V = check_overflow_add<std::int16_t>(a, b);
			sr.C = check_carry<std::uint16_t>(a, b);
			sr.N = (std::int16_t)res < 0;
		}
		else
		{
			res = a + b;
			sr.V = check_overflow_add<std::int32_t>(a, b);
			sr.C = check_carry<std::uint32_t>(a, b);
			sr.N = (std::int32_t)res < 0;
		}

		sr.X = sr.C;
		sr.Z = res == 0;
		return res;
	}

	template <class T>
	static std::uint8_t check_overflow_add(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		auto sum = (std::int64_t)a + (std::int64_t)b + c;

		if (sum > std::numeric_limits<T>::max())
			return 1;

		if (sum < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	template <class T>
	static std::uint8_t check_carry(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);

		auto sum = (std::int64_t)a + (std::int64_t)b + c;
		if (sum > std::numeric_limits<T>::max())
			return 1;

		return 0;
	}

private:
	static std::uint32_t value(data_register& reg, std::uint8_t size)
	{
		if(size == 1)
			return reg.B;
		else if(size == 2)
			return reg.W;
		else
			return reg.LW;
	}

	static std::uint32_t value(operand& op, std::uint8_t size)
	{
		if(size == 1)
		{
			if(op.is_data_reg())
				return op.data_reg().B;
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}
		else if(size == 2)
		{
			if(op.is_data_reg())
				return op.data_reg().W;
			else if(op.is_addr_reg())
				return op.addr_reg().W;
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}
		else if(size == 4)
		{
			if(op.is_data_reg())
				return op.data_reg().LW;
			else if(op.is_addr_reg())
				return op.addr_reg().LW;
			else if(op.is_imm())
				return op.imm();
			else if(op.is_pointer())
				return op.pointer().value;
		}

		throw internal_error();
	}
};

}

#endif // __M68K_OPERATIONS_HPP__
