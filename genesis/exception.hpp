#ifndef __EXCEPTION_HPP__
#define __EXCEPTION_HPP__

#include <exception>
#include <string_view>
#include <sstream>
#include <source_location>


namespace genesis
{

namespace __impl
{
static std::string build_msg(std::string_view error_type, std::string_view msg, const std::source_location &location)
{
	std::stringstream ss;
	ss << location.function_name() << " at " << location.line();
	ss << ": " << error_type;
	if(msg.size() != 0)
		ss << '(' << msg << ')';
	return ss.str();
}
}

class internal_error : public std::runtime_error
{
public:
	internal_error(std::string_view msg = "",
		const std::source_location &loc = std::source_location::current())
		: std::runtime_error(__impl::build_msg("internal error", msg, loc))
	{
	}
};

class not_implemented : public std::runtime_error
{
public:
	not_implemented(std::string_view msg = "",
		const std::source_location &loc = std::source_location::current())
		: std::runtime_error(__impl::build_msg("not implemented", msg, loc))
	{
	}
};

};

#endif // __EXCEPTION_HPP__
