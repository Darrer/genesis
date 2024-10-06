#ifndef __MEMORY_LOGGING_MEMORY_H__
#define __MEMORY_LOGGING_MEMORY_H__

#include "addressable.h"
#include "string_utils.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace genesis::memory
{

class logging_memory : public memory::addressable
{
public:
	logging_memory(std::shared_ptr<addressable> unit, std::ostream& os, std::string_view name)
		: m_unit(unit), m_os(os), m_name(name)
	{
	}

	std::uint32_t max_address() const override
	{
		return m_unit->max_address();
	}

	bool is_idle() const override
	{
		return m_unit->is_idle();
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		m_os << m_name << ": writing byte at " << su::hex_str(address) << ", data " << su::hex_str(data) << '\n';
		m_unit->init_write(address, data);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		m_os << m_name << ": writing word at " << su::hex_str(address) << ", data " << su::hex_str(data) << '\n';
		m_unit->init_write(address, data);
	}

	void init_read_byte(std::uint32_t address) override
	{
		m_os << m_name << ": reading byte at " << su::hex_str(address) << '\n';
		m_unit->init_read_byte(address);
	}

	void init_read_word(std::uint32_t address) override
	{
		m_os << m_name << ": reading word at " << su::hex_str(address) << '\n';
		m_unit->init_read_word(address);
	}

	std::uint8_t latched_byte() const override
	{
		std::uint8_t data = m_unit->latched_byte();
		m_os << m_name << ": latching byte: " << su::hex_str(data) << '\n';
		return data;
	}

	std::uint16_t latched_word() const override
	{
		std::uint16_t data = m_unit->latched_word();
		m_os << m_name << ": latching word: " << su::hex_str(data) << '\n';
		return data;
	}

private:
	std::shared_ptr<addressable> m_unit;
	std::ostream& m_os;
	std::string m_name;
};

} // namespace genesis::memory

#endif // __MEMORY_LOGGING_MEMORY_H__
