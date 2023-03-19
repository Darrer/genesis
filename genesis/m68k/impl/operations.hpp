#ifndef __M68K_OPERATIONS_HPP__
#define __M68K_OPERATIONS_HPP__

#include <limits>

#include "instruction_type.h"
#include "m68k/cpu_registers.hpp"
#include "ea_decoder.hpp"

#include "exception.hpp"


namespace genesis::m68k
{

class operations
{
public:
	operations() = delete;

	template<class T1, class T2>
	static std::uint32_t add(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return add(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t sub(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return sub(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t and_op(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return and_op(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t or_op(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return or_op(value(a, size), value(b, size), size, sr);
	}

	template<class T1, class T2>
	static std::uint32_t eor(T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		return eor(value(a, size), value(b, size), size, sr);
	}

	/* helpers */
	template<class T1, class T2>
	static std::uint32_t alu(inst_type inst, T1 a, T2 b, std::uint8_t size, status_register& sr)
	{
		switch (inst)
		{
		case inst_type::ADD:
		case inst_type::ADDI:
		case inst_type::ADDQ:
			return operations::add(a, b, size, sr);

		case inst_type::SUB:
		case inst_type::SUBI:
		case inst_type::SUBQ:
			return operations::sub(a, b, size, sr);

		case inst_type::AND:
		case inst_type::ANDI:
			return operations::and_op(a, b, size, sr);

		case inst_type::OR:
		case inst_type::ORI:
			return operations::or_op(a, b, size, sr);

		case inst_type::EOR:
		case inst_type::EORI:
			return operations::eor(a, b, size, sr);

		default: throw internal_error();
		}
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

	static std::uint32_t sub(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res;

		if(size == 1)
		{
			res = std::uint8_t(a - b);
			sr.V = check_overflow_sub<std::int8_t>(a, b);
			sr.C = check_borrow<std::uint8_t>(a, b);
			sr.N = (std::int8_t)res < 0;
		}
		else if(size == 2)
		{
			res = std::uint16_t(a - b);
			sr.V = check_overflow_sub<std::int16_t>(a, b);
			sr.C = check_borrow<std::uint16_t>(a, b);
			sr.N = (std::int16_t)res < 0;
		}
		else
		{
			res = a - b;
			sr.V = check_overflow_sub<std::int32_t>(a, b);
			sr.C = check_borrow<std::uint32_t>(a, b);
			sr.N = (std::int32_t)res < 0;
		}

		sr.X = sr.C;
		sr.Z = res == 0;
		return res;
	}

	static std::uint32_t and_op(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a & b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t or_op(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a | b;
		set_logical_flags(res, size, sr);
		return res;
	}

	static std::uint32_t eor(std::uint32_t a, std::uint32_t b, std::uint8_t size, status_register& sr)
	{
		std::uint32_t res = a ^ b;
		set_logical_flags(res, size, sr);
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

	template <class T>
	static std::uint8_t check_overflow_sub(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == true);

		auto diff = (std::int64_t)a - (std::int64_t)b - c;

		if (diff > std::numeric_limits<T>::max())
			return 1;

		if (diff < std::numeric_limits<T>::min())
			return 1;

		return 0;
	}

	template <class T>
	static std::uint8_t check_borrow(T a, T b, std::uint8_t c = 0)
	{
		static_assert(std::numeric_limits<T>::is_signed == false);

		auto sum = (std::int64_t)b + c;
		if (sum > a)
			return 1;

		return 0;
	}

	static void set_logical_flags(std::uint32_t res, std::uint8_t size, status_register& sr)
	{
		if(size == 1)
		{
			res = std::uint8_t(res);
			sr.N = (std::int8_t)res < 0;
		}
		else if(size == 2)
		{
			res = std::uint16_t(res);
			sr.N = (std::int16_t)res < 0;
		}
		else
		{
			sr.N = (std::int32_t)res < 0;
		}

		sr.C = sr.V = 0;
		sr.Z = res == 0;
	}

private:
	static std::uint32_t value(data_register reg, std::uint8_t size)
	{
		if(size == 1)
			return reg.B;
		else if(size == 2)
			return reg.W;
		else
			return reg.LW;
	}

	static std::uint32_t value(address_register reg, std::uint8_t size)
	{
		if(size == 2)
			return reg.W;
		else if(size == 4)
			return reg.LW;
		
		throw internal_error();
	}

	static std::uint32_t value(std::uint32_t val, std::uint8_t /* size */)
	{
		return val;
	}

	static std::uint32_t value(operand& op, std::uint8_t size)
	{
		if(op.is_imm()) return value(op.imm(), size);
		if(op.is_pointer()) return value(op.pointer().value, size);
		if(op.is_data_reg()) return value(op.data_reg(), size);
		if(op.is_addr_reg()) return value(op.addr_reg(), size);

		throw internal_error();
	}
};

}

#endif // __M68K_OPERATIONS_HPP__
