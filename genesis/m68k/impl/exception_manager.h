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
	address_error,
	bus_error,
	trap,
	trapv,
	division_by_zero,
	privilege_violations,
	count
};

struct address_error
{
	std::uint32_t address;
	std::uint32_t PC;
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

	void rise_division_by_zero()
	{
		rise(exception_type::division_by_zero);
	}

	void accept_division_by_zero()
	{
		accept(exception_type::division_by_zero);
	}

	void rise_privilege_violations()
	{
		rise(exception_type::privilege_violations);
	}

	void accept_privilege_violations()
	{
		accept(exception_type::privilege_violations);
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

	void accept_all()
	{
		exps.reset();
	}

private:
	std::bitset<static_cast<std::uint8_t>(exception_type::count)> exps;
	std::optional<address_error> addr_error;
	std::optional<std::uint8_t> trap_vector;
};

}

#endif // __M68K_EXCEPTION_MANAGER_H__
