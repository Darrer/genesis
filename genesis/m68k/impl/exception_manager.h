#ifndef __M68K_EXCEPTION_MANAGER_H__
#define __M68K_EXCEPTION_MANAGER_H__

namespace genesis::m68k
{

class exception_manager
{
public:
	void rise_address_error(std::uint32_t /*addr*/, std::uint32_t /*pc*/,
		std::uint8_t /*rw*/, std::uint8_t /*in*/)
	{

	}
};

}

#endif // __M68K_EXCEPTION_MANAGER_H__
