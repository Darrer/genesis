#ifndef __M68K_SIZE_TYPE_H__
#define __M68K_SIZE_TYPE_H__

namespace genesis::m68k
{

enum class size_type
{
	BYTE = 1,
	WORD = 2,
	LONG = 4,
};

static unsigned int size_in_bytes(size_type size)
{
	if(size == size_type::BYTE)
		return 1;
	if(size == size_type::WORD)
		return 2;
	return 4;
}

}

#endif // __M68K_SIZE_TYPE_H__
