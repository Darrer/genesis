#include "memory_builder.h"

#include "exception.hpp"
#include "string_utils.hpp"

#include <stdexcept>
#include <optional>


namespace genesis::memory
{

class composite_memory : public addressable
{
private:
	struct addressable_device
	{
		addressable& memory_unit;
		std::uint32_t start_address;
		std::uint32_t end_address;
	};

public:

	/* Implement addressable interface */

	std::uint32_t capacity() const override
	{
		if(refs.size() == 0)
			return 0;

		std::uint32_t max_address = 0;
		for(auto& dev : refs)
			max_address = std::max(max_address, dev.end_address);

		return max_address + 1;
	}
	
	bool is_idle() const override
	{
		if(last_device.has_value() == false)
		{
			// there were not requests so far
			return true;
		}

		return last_device.value().memory_unit.is_idle();
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.init_write(address, data);

		last_device.emplace(dev);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		assert_idle();
		
		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.init_write(address, data);

		last_device.emplace(dev);
	}

	void init_read_byte(std::uint32_t address) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.init_read_byte(address);

		last_device.emplace(dev);
	}

	void init_read_word(std::uint32_t address) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.init_read_word(address);

		last_device.emplace(dev);
	}

	std::uint8_t latched_byte() const override
	{
		return last_device.value().memory_unit.latched_byte();
	}

	std::uint16_t latched_word() const override
	{
		return last_device.value().memory_unit.latched_word();
	}

	/* Composite methods */

	void add(addressable& memory_unit, std::uint32_t start_address, std::uint32_t end_address)
	{
		refs.push_back({memory_unit, start_address, end_address});
	}

	void add(std::shared_ptr<addressable> memory_unit, std::uint32_t start_address, std::uint32_t end_address)
	{
		dev_shared_ptrs.push_back(memory_unit);
		refs.push_back({*memory_unit, start_address, end_address});
	}

private:
	void assert_idle() const
	{
		if(is_idle() == false)
		{
			throw internal_error();
		}
	}

	addressable_device find_device(std::uint32_t address)
	{
		for(auto& dev : refs)
		{
			if(dev.start_address >= address && dev.end_address <= address)
				return dev;
		}

		throw internal_error("cannot find addressable device serving address 0x" + su::hex_str(address));
	}

	std::uint32_t convert_address(addressable_device dev, std::uint32_t address)
	{
		return address - dev.start_address;
	}

private:
	std::vector<addressable_device> refs;
	std::vector<std::shared_ptr<addressable>> dev_shared_ptrs; // just keep them to prevent deallocation

	std::optional<addressable_device> last_device;
};


std::shared_ptr<addressable> memory_builder::build() const
{
	std::shared_ptr<composite_memory> comp = std::make_shared<composite_memory>();

	for(auto& dev : refs)
		comp->add(dev.memory_unit, dev.start_address, dev.end_address);

	for(auto& dev : shared_ptrs)
		comp->add(dev.memory_unit, dev.start_address, dev.end_address);

	return comp;
}

void memory_builder::add(addressable& memory_unit, std::uint32_t start_address, std::uint32_t end_address)
{
	check_args(memory_unit, start_address, end_address);

	refs.push_back({memory_unit, start_address, end_address});
}
	
void memory_builder::add(std::shared_ptr<addressable> memory_unit, std::uint32_t start_address, std::uint32_t end_address)
{
	if(memory_unit == nullptr)
		throw std::invalid_argument("memory unit cannot be nullptr");

	check_args(*memory_unit, start_address, end_address);

	shared_ptrs.push_back({std::move(memory_unit), start_address, end_address});
}

void memory_builder::check_args(addressable& memory_unit, std::uint32_t start_address, std::uint32_t end_address)
{
	if(end_address < start_address)
		throw std::invalid_argument("end_address cannot be less than start_address");

	std::uint32_t capacity = end_address - start_address + 1; // +1 as address range is [start ; end]
	if(memory_unit.capacity() < capacity)
		throw std::invalid_argument("provided addressable device cannot address specified capacity");
}

}
