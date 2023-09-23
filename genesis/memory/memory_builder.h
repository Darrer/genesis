#ifndef __MEMORY_BUILDER_H__
#define __MEMORY_BUILDER_H__

#include "addressable.h"
#include <memory>
#include <vector>


namespace genesis::memory
{

/* Combine multiple addressable devices into single address space */
class memory_builder
{
private:
	struct memory_ref
	{
		addressable& memory_unit;
		std::uint32_t start_address;
		std::uint32_t end_address;
	};

	struct memory_shared_ptr
	{
		std::shared_ptr<addressable> memory_unit;
		std::uint32_t start_address;
		std::uint32_t end_address;
	};

public:
	memory_builder() = default;

	std::shared_ptr<addressable> build() const;

	void add(addressable& memory_unit, std::uint32_t start_address);
	void add(addressable& memory_unit, std::uint32_t start_address, std::uint32_t end_address);

	void add(std::shared_ptr<addressable> memory_unit, std::uint32_t start_address);
	void add(std::shared_ptr<addressable> memory_unit, std::uint32_t start_address, std::uint32_t end_address);

private:
	void check_args(addressable& memory_unit, std::uint32_t start_address, std::uint32_t end_address);
	void check_intersect(std::uint32_t start_address, std::uint32_t end_address);

private:
	std::vector<memory_ref> refs;
	std::vector<memory_shared_ptr> shared_ptrs;
};

};

#endif // __MEMORY_BUILDER_H__
