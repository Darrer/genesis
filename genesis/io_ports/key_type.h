#ifndef __IO_PORTS_KEY_TYPE_H__
#define __IO_PORTS_KEY_TYPE_H__

#include "exception.hpp"

#include <string_view>
#include <type_traits>

namespace genesis::io_ports
{

enum class key_type
{
	UP,
	DOWN,
	LEFT,
	RIGHT,
	START,
	MODE,

	A,
	B,
	C,
	X,
	Y,
	Z,
};

static constexpr const int key_type_count = 12;

[[maybe_unused]]
static constexpr std::underlying_type_t<enum key_type> key_type_index(key_type key)
{
	return static_cast<std::underlying_type_t<key_type>>(key);
}

[[maybe_unused]]
static std::string_view key_type_name(key_type key)
{
	switch(key)
	{
	case key_type::UP:
		return "UP";
	case key_type::DOWN:
		return "DOWN";
	case key_type::LEFT:
		return "LEFT";
	case key_type::RIGHT:
		return "RIGHT";
	case key_type::START:
		return "START";
	case key_type::MODE:
		return "MODE";
	case key_type::A:
		return "A";
	case key_type::B:
		return "B";
	case key_type::C:
		return "C";
	case key_type::X:
		return "X";
	case key_type::Y:
		return "Y";
	case key_type::Z:
		return "Z";
	default:
		throw genesis::internal_error();
	}
}

} // namespace genesis::io_ports

#endif // __IO_PORTS_KEY_TYPE_H__
