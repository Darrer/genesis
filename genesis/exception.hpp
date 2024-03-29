#ifndef __EXCEPTION_HPP__
#define __EXCEPTION_HPP__

#include <exception>
#include <source_location>
#include <sstream>
#include <string_view>
#include <stacktrace>


namespace genesis
{

namespace __impl
{
static std::string build_msg(std::string_view error_type, std::string_view msg,
	const std::source_location& location, const std::stacktrace& strace)
{
	std::stringstream ss;
	ss << location.function_name() << " at " << location.file_name() << ':' << location.line();
	ss << ' ' << error_type;

	if(msg.size() != 0)
		ss << '(' << msg << ')';

	ss << '\n' << strace;

	return ss.str();
}
} // namespace __impl

class internal_error : public std::runtime_error
{
public:
	internal_error(std::string_view msg = "",
		const std::source_location loc = std::source_location::current(),
		const std::stacktrace st = std::stacktrace::current())
			: std::runtime_error(__impl::build_msg("internal error", msg, loc, st))
	{
	}
};

class not_implemented : public std::runtime_error
{
public:
	not_implemented(std::string_view msg = "",
		const std::source_location loc = std::source_location::current(),
		const std::stacktrace st = std::stacktrace::current())
			: std::runtime_error(__impl::build_msg("not implemented", msg, loc, st))
	{
	}
};

}; // namespace genesis

#endif // __EXCEPTION_HPP__
