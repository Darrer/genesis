#include "memory_builder.h"

#include "exception.hpp"
#include "string_utils.hpp"

#include <optional>
#include <stdexcept>
#include <functional>


namespace genesis::memory
{

struct addressable_device
{
	std::reference_wrapper<addressable> memory_unit;

	std::uint32_t start_address;
	std::uint32_t end_address;
};

bool is_intersect(const auto& device, std::uint32_t address)
{
	return device.start_address <= address && address <= device.end_address;
}

class composite_memory : public addressable
{
public:

	/* Addressable interface */

	std::uint32_t max_address() const override
	{
		std::uint32_t max_address = 0;
		for(auto& dev : m_refs)
			max_address = std::max(max_address, dev.end_address);
		return max_address;
	}

	bool is_idle() const override
	{
		if(m_last_device.has_value() == false)
		{
			// there were not requests so far
			return true;
		}

		return m_last_device.value().memory_unit.get().is_idle();
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.get().init_write(address, data);

		m_last_device.emplace(dev);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.get().init_write(address, data);

		m_last_device.emplace(dev);
	}

	void init_read_byte(std::uint32_t address) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.get().init_read_byte(address);

		m_last_device.emplace(dev);
	}

	void init_read_word(std::uint32_t address) override
	{
		assert_idle();

		auto dev = find_device(address);
		address = convert_address(dev, address);
		dev.memory_unit.get().init_read_word(address);

		m_last_device.emplace(dev);
	}

	std::uint8_t latched_byte() const override
	{
		return m_last_device.value().memory_unit.get().latched_byte();
	}

	std::uint16_t latched_word() const override
	{
		return m_last_device.value().memory_unit.get().latched_word();
	}

	/* Composite interface */

	void save_devices(std::vector<addressable_device> devices)
	{
		m_refs = std::move(devices);
		m_refs.shrink_to_fit();
	}

	void save_ptrs(std::vector<std::shared_ptr<addressable>> ptrs)
	{
		m_shared_ptrs = std::move(ptrs);
		m_shared_ptrs.shrink_to_fit();
	}

	void save_ptrs(std::vector<std::unique_ptr<addressable>> ptrs)
	{
		m_unique_ptrs = std::move(ptrs);
		m_unique_ptrs.shrink_to_fit();
	}

private:
	void assert_idle() const
	{
		if(is_idle() == false)
		{
			throw internal_error();
		}
	}

	addressable_device find_device(std::uint32_t address) const
	{
		for(auto& dev : m_refs)
		{
			if(dev.start_address <= address && address <= dev.end_address)
				return dev;
		}

		throw std::runtime_error("cannot find addressable device serving address " + su::hex_str(address));
	}

	static std::uint32_t convert_address(addressable_device dev, std::uint32_t address)
	{
		return address - dev.start_address;
	}

private:
	std::vector<addressable_device> m_refs;

	// keep ptrs to prevent deallocation
	std::vector<std::shared_ptr<addressable>> m_shared_ptrs;
	std::vector<std::unique_ptr<addressable>> m_unique_ptrs;

	std::optional<addressable_device> m_last_device;
};


std::shared_ptr<addressable> memory_builder::build()
{
	if(m_refs.empty() && m_shared_ptrs.empty() && m_unique_ptrs.empty())
		throw internal_error("tried to build without devices");

	std::vector<addressable_device> devices;
	devices.reserve(m_refs.size());

	for(auto& dev : m_refs)
		devices.push_back({dev.memory_unit, dev.start_address, dev.end_address});

	// Place devices with larger capacities at the beginning of the array
	std::ranges::sort(devices, [](const addressable_device& a, const addressable_device& b)
	{
		auto a_capacity = a.end_address - a.start_address;
		auto b_capacity = b.end_address - b.start_address;
		return b_capacity < a_capacity;
	});

	std::shared_ptr<composite_memory> comp = std::make_shared<composite_memory>();
	
	comp->save_devices(std::move(devices));
	comp->save_ptrs(std::move(m_shared_ptrs));
	comp->save_ptrs(std::move(m_unique_ptrs));

	m_refs.clear();
	m_shared_ptrs.clear();
	m_unique_ptrs.clear();

	return comp;
}

void memory_builder::add(addressable& device, std::uint32_t start_address)
{
	add(device, start_address, start_address + device.max_address());
}

void memory_builder::add(addressable& device, std::uint32_t start_address, std::uint32_t end_address)
{
	check_args(device, start_address, end_address);

	m_refs.push_back({device, start_address, end_address});
}

void memory_builder::add(std::shared_ptr<addressable> device, std::uint32_t start_address)
{
	assert_not_null(device.get());
	add(device, start_address, start_address + device->max_address());
}

void memory_builder::add(std::shared_ptr<addressable> device, std::uint32_t start_address,
						 std::uint32_t end_address)
{
	check_args(device.get(), start_address, end_address);

	m_refs.push_back({*device, start_address, end_address});
	m_shared_ptrs.push_back(std::move(device));
}

void memory_builder::add(std::unique_ptr<addressable> device, std::uint32_t start_address)
{
	assert_not_null(device.get());

	std::uint32_t end_address = start_address + device->max_address();
	add(std::move(device), start_address, end_address);
}

void memory_builder::add(std::unique_ptr<addressable> device, std::uint32_t start_address,
	std::uint32_t end_address)
{
	check_args(device.get(), start_address, end_address);

	m_refs.push_back({*device, start_address, end_address});
	m_unique_ptrs.push_back(std::move(device));
}

void memory_builder::mirror(std::uint32_t start_address, std::uint32_t end_address,
		std::uint32_t mirrored_start_address, std::uint32_t mirrored_end_address)
{
	if((end_address - start_address) != (mirrored_end_address - mirrored_start_address))
		throw std::invalid_argument("address ranges must have the same size");

	check_intersect(mirrored_start_address, mirrored_end_address);

	// check that start_address and end_address belong to the same device
	for(const auto& device : m_refs)
	{
		if((is_intersect(device, start_address) && !is_intersect(device, end_address))
			|| (is_intersect(device, end_address) && !is_intersect(device, start_address)))
		{
			throw std::invalid_argument("start/end addresses must belong to the same device");
		}
	}

	const auto& device = std::ranges::find_if(m_refs, [start_address, end_address](const auto& dev)
	{
		return is_intersect(dev, start_address) && is_intersect(dev, end_address);
	});

	if(device == m_refs.end())
		throw std::invalid_argument("cannot find device serving inital addresses");

	memory_ref new_device {device->memory_unit, mirrored_start_address, mirrored_end_address};
	m_refs.push_back(new_device);
}

void memory_builder::assert_not_null(addressable* device) const
{
	if(device == nullptr)
		throw std::invalid_argument("addressable device cannot be null");
}

void memory_builder::check_args(addressable* device, std::uint32_t start_address, std::uint32_t end_address) const
{
	assert_not_null(device);
	check_args(*device, start_address, end_address);
}

void memory_builder::check_args(addressable& device, std::uint32_t start_address, std::uint32_t end_address) const
{
	if(end_address < start_address)
		throw std::invalid_argument("end_address cannot be less than start_address");

	std::uint32_t max_address = end_address - start_address;

	// TODO: temporary disable this check
	if(device.max_address() != max_address && false)
		throw std::invalid_argument("provided addressable device has unexpected space size");

	check_intersect(start_address, end_address);
}

void memory_builder::check_intersect(std::uint32_t start_address, std::uint32_t end_address) const
{
	auto is_intersect = [start_address, end_address](const auto& unit) {
		return (unit.start_address <= start_address && start_address <= unit.end_address) ||
			   (unit.start_address <= end_address && end_address <= unit.end_address);
	};

	if(std::ranges::any_of(m_refs, is_intersect))
		throw std::invalid_argument("specified address range intersects with existing device");
}

} // namespace genesis::memory
