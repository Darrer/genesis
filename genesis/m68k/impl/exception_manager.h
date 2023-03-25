#ifndef __M68K_EXCEPTION_MANAGER_H__
#define __M68K_EXCEPTION_MANAGER_H__

#include <optional>
#include <bitset>
#include "exception.hpp"

namespace genesis::m68k
{

enum class exception_type : std::uint8_t
{
	none,
	address_error,
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

class exception_manager
{
public:
	void rise(exception_type ex)
	{
		// mulltiple exception of the same type are not allowed
		if(is_raised(ex))
			throw internal_error();
		exps.set(static_cast<std::uint8_t>(ex), true);
	}

	bool is_raised(exception_type ex) const
	{
		return exps.test(static_cast<std::uint8_t>(ex));
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

private:
	std::bitset<static_cast<std::uint8_t>(exception_type::count)> exps;
	std::optional<address_error> addr_error;
};

}

#endif // __M68K_EXCEPTION_MANAGER_H__
