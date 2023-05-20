#ifndef __M68K_EXCEPTION_MANAGER_H__
#define __M68K_EXCEPTION_MANAGER_H__

#include <array>
#include <type_traits>
#include <span>
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
	line_1010_emulator,
	line_1111_emulator,

	/* 2 group */
	trap,
	trapv,
	chk_instruction,
	division_by_zero,

	count
};


enum class exception_group
{
	group_0,
	group_1,
	group_2,
};

namespace groups
{
static constexpr const exception_type ex_group_0[] = { exception_type::reset,
	exception_type::address_error, exception_type::bus_error };

static constexpr const exception_type ex_group_1[] = { exception_type::trace, exception_type::interrupt,
	exception_type::illegal_instruction, exception_type::privilege_violations,
	exception_type::line_1010_emulator, exception_type::line_1111_emulator, };


static constexpr const exception_type ex_group_2[] = { exception_type::trap, exception_type::trapv,
	exception_type::chk_instruction, exception_type::division_by_zero };
};

static constexpr std::span<const exception_type> group_exceptions(exception_group group)
{
	switch (group)
	{
	case exception_group::group_0:
		return groups::ex_group_0;

	case exception_group::group_1:
		return groups::ex_group_1;

	case exception_group::group_2:
		return groups::ex_group_2;
	
	default: return { };
	}
}


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
private:
	using index_type = std::underlying_type_t<exception_type>;

public:
	exception_manager()
	{
		exps.fill(false);
	}

	bool is_raised(exception_type ex) const
	{
		return exps[index(ex)];
	}

	bool is_raised(exception_group group) const
	{
		for(auto ex : group_exceptions(group))
		{
			if(is_raised(ex))
				return true;
		}
		return false;
	}

	bool is_raised_any() const
	{
		return ex_counter != 0;
	}

	void accept(exception_type ex)
	{
		if(!is_raised(ex))
			throw internal_error();

		exps[index(ex)] = false;
		--ex_counter;
	}

	void accept_all()
	{
		exps.fill(false);
	}

	void rise(exception_type ex)
	{
		// some exceptions required data to be provided, so we cannot allow use generic rise method for them
		switch (ex)
		{
		case exception_type::address_error:
		case exception_type::bus_error:
		case exception_type::trap:
			throw internal_error("Specialized rise method should be used for this exception");
		}

		rise_unsafe(ex);
	}

	// TOOD: we've got too many methods

	/* Group 0 */

	void rise_reset()
	{
		rise_unsafe(exception_type::reset);
	}

	void rise_address_error(address_error _addr_error)
	{
		rise_unsafe(exception_type::address_error);
		addr_error = _addr_error;
	}

	address_error accept_address_error()
	{
		accept(exception_type::address_error);
		return addr_error;
	}

	void rise_bus_error(bus_error _addr_error)
	{
		rise_unsafe(exception_type::bus_error);
		addr_error = _addr_error;
	}

	bus_error accept_bus_error()
	{
		accept(exception_type::bus_error);
		return addr_error;
	}

	/* Group 1 */

	void rise_trace()
	{
		rise_unsafe(exception_type::trace);
	}

	void rise_interrupt()
	{
		rise_unsafe(exception_type::interrupt);
	}

	void rise_illegal_instruction()
	{
		rise_unsafe(exception_type::illegal_instruction);
	}

	void rise_privilege_violations()
	{
		rise_unsafe(exception_type::privilege_violations);
	}

	void rise_line_1010_emulator()
	{
		rise_unsafe(exception_type::line_1010_emulator);
	}

	void rise_line_1111_emulator()
	{
		rise_unsafe(exception_type::line_1111_emulator);
	}

	/* Group 2 */

	void rise_trap(std::uint8_t vector)
	{
		rise_unsafe(exception_type::trap);
		trap_vector = vector;
	}

	std::uint8_t accept_trap()
	{
		accept(exception_type::trap);
		return trap_vector;
	}

	void rise_trapv()
	{
		rise_unsafe(exception_type::trapv);
	}

	void rise_chk_instruction()
	{
		rise_unsafe(exception_type::chk_instruction);
	}

	void rise_division_by_zero()
	{
		rise_unsafe(exception_type::division_by_zero);
	}

private:
	void rise_unsafe(exception_type ex)
	{
		if(is_raised(ex))
			throw not_implemented("multiple exceptions of the same type are not allowed yet");

		exps[index(ex)] = true;
		++ex_counter;
	}

	static constexpr index_type index(exception_type exp)
	{
		return static_cast<index_type>(exp);
	}

private:
	std::array<bool, static_cast<index_type>(exception_type::count)> exps;
	address_error addr_error;
	std::uint8_t trap_vector;
	unsigned int ex_counter = 0;
};

}

#endif // __M68K_EXCEPTION_MANAGER_H__
