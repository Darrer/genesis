#ifndef __IO_PORTS_KEY_TYPE_H__
#define __IO_PORTS_KEY_TYPE_H__

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

	A, B, C,
	X, Y, Z,
};

static constexpr const int key_type_count = 12;

static constexpr std::underlying_type_t<enum key_type> key_type_index(key_type key)
{
	return static_cast<std::underlying_type_t<key_type>>(key);
}

}

#endif // __IO_PORTS_KEY_TYPE_H__
