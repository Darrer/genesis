#ifndef __MEMORY_MEMORY_BUILDER_H__
#define __MEMORY_MEMORY_BUILDER_H__

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

	struct memory_unique_ptr
	{
		std::unique_ptr<addressable> memory_unit;
		std::uint32_t start_address;
		std::uint32_t end_address;
	};

public:
	memory_builder() = default;

	std::shared_ptr<addressable> build();

	// TODO: saving reference doesn't seem safe
	void add(addressable& device, std::uint32_t start_address);
	void add(addressable& device, std::uint32_t start_address, std::uint32_t end_address);

	void add(std::shared_ptr<addressable> device, std::uint32_t start_address);
	void add(std::shared_ptr<addressable> device, std::uint32_t start_address, std::uint32_t end_address);

	void add(std::unique_ptr<addressable> device, std::uint32_t start_address);
	void add(std::unique_ptr<addressable> device, std::uint32_t start_address, std::uint32_t end_address);

	void add_unique(std::unique_ptr<addressable> device, std::uint32_t start_address)
	{
		add(std::move(device), start_address);
	}

	void add_unique(std::unique_ptr<addressable> device, std::uint32_t start_address, std::uint32_t end_address)
	{
		add(std::move(device), start_address, end_address);
	}

	// Mirror [start_address; end_address] on [mirrored_start_address; mirrored_end_address]
	// These address ranges must have the same size.
	void mirror(std::uint32_t start_addres, std::uint32_t end_address,
		std::uint32_t mirrored_start_address, std::uint32_t mirrored_end_address);

private:
	void assert_not_null(addressable* device) const;
	void check_args(addressable* device, std::uint32_t start_address, std::uint32_t end_address) const;
	void check_args(addressable& device, std::uint32_t start_address, std::uint32_t end_address) const;
	void check_intersect(std::uint32_t start_address, std::uint32_t end_address) const;

private:
	std::vector<memory_ref> m_refs;
	std::vector<std::shared_ptr<addressable>> m_shared_ptrs;
	std::vector<std::unique_ptr<addressable>> m_unique_ptrs;
};

}; // namespace genesis::memory

#endif // __MEMORY_MEMORY_BUILDER_H__
