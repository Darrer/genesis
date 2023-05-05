#ifndef __M68K_EXCEPTION_MANAGER_H__
#define __M68K_EXCEPTION_MANAGER_H__

#include <optional>
#include <bitset>
#include "exception.hpp"

namespace genesis::m68k
{

enum class exception_type
{
	none,

	/* 0 group */
	reset,
	address_error,
	bus_error,

	/* 1 group */
	trace,
	interrupt,
	illegal_instruction,
	privilege_violations,

	/* 2 group */
	trap,
	trapv,
	chk_instruction,
	division_by_zero,

	count
};

struct address_error
{
	std::uint32_t address;
	std::uint8_t func_codes;
	bool rw;
	bool in;
};

using bus_error = address_error;

class exception_manager
{
public:
	bool is_raised(exception_type ex) const
	{
		return exps.test(static_cast<std::uint8_t>(ex));
	}

	void accept_all()
	{
		exps.reset();
	}

	/* Group 0 */

	void rise_reset()
	{
		rise(exception_type::reset);
	}

	void accept_reset()
	{
		accept(exception_type::reset);
	}

	void rise_address_error(address_error _addr_error)
	{
		rise(exception_type::address_error);
		addr_error = _addr_error;
	}

	address_error accept_address_error()
	{
		accept(exception_type::address_error);
		return addr_error.value();
	}

	void rise_bus_error(bus_error _addr_error)
	{
		rise(exception_type::bus_error);
		addr_error = _addr_error;
	}

	bus_error accept_bus_error()
	{
		accept(exception_type::bus_error);
		return addr_error.value();
	}

	/* Group 1 */

	void rise_trace()
	{
		rise(exception_type::trace);
	}

	void accept_trace()
	{
		accept(exception_type::trace);
	}

	void rise_interrupt()
	{
		rise(exception_type::interrupt);
	}

	void accept_interrupt()
	{
		accept(exception_type::interrupt);
	}

	void rise_illegal_instruction()
	{
		rise(exception_type::illegal_instruction);
	}

	void accept_illegal_instruction()
	{
		accept(exception_type::illegal_instruction);
	}

	void rise_privilege_violations()
	{
		rise(exception_type::privilege_violations);
	}

	void accept_privilege_violations()
	{
		accept(exception_type::privilege_violations);
	}

	/* Group 2 */

	void rise_trap(std::uint8_t vector)
	{
		rise(exception_type::trap);
		trap_vector = vector;
	}

	std::uint8_t accept_trap()
	{
		accept(exception_type::trap);
		return trap_vector.value();
	}

	void rise_trapv()
	{
		rise(exception_type::trapv);
	}

	void accept_trapv()
	{
		accept(exception_type::trapv);
	}

	void rise_chk_instruction()
	{
		rise(exception_type::chk_instruction);
	}

	void accept_chk_instruction()
	{
		accept(exception_type::chk_instruction);
	}

	void rise_division_by_zero()
	{
		rise(exception_type::division_by_zero);
	}

	void accept_division_by_zero()
	{
		accept(exception_type::division_by_zero);
	}

private:
	void rise(exception_type ex)
	{
		if(is_raised(ex))
			throw not_implemented("multiple exceptions of the same type are not allowed yet");

		exps.set(static_cast<std::uint8_t>(ex), true);
	}

	void accept(exception_type ex)
	{
		if(!is_raised(ex))
			throw internal_error();

		exps.set(static_cast<std::uint8_t>(ex), false);
	}

private:
	std::bitset<static_cast<std::uint8_t>(exception_type::count)> exps;
	std::optional<address_error> addr_error;
	std::optional<std::uint8_t> trap_vector;
};

}

#endif // __M68K_EXCEPTION_MANAGER_H__
