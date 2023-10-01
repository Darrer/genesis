#ifndef __GENESIS_H__
#define __GENESIS_H__

#include "memory/addressable.h"

#include <memory>

namespace genesis
{

class genesis
{
public:
	genesis() = default;

private:
	void build_cpu_memory_map();

private:
	std::shared_ptr<memory::addressable> m68k_mem_map;
	// std::shared_ptr<memory::addressable> z80_mem_map;
};

} // namespace genesis

#endif // __GENESIS_H__
