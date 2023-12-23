#ifndef __MEMORY_BASE_UNIT_H__
#define __MEMORY_BASE_UNIT_H__

#include <span>
#include <cstdint>
#include <optional>

#include "addressable.h"
#include "endian.hpp"
#include "exception.hpp"
#include "string_utils.hpp"

namespace genesis::memory
{

class base_unit : public addressable
{
protected:
	base_unit(std::span<std::uint8_t> buffer, std::endian byte_order = std::endian::native)
		: m_buffer(buffer), m_byte_order(byte_order)
	{
		if(m_buffer.size() == 0)
			throw genesis::internal_error();
	}

public:
	/* addressable interface */

	std::uint32_t max_address() const override
	{
		return static_cast<std::uint32_t>(m_buffer.size() - 1);
	}

	bool is_idle() const override
	{
		// always idle
		return true;
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		reset();
		write(address, data);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		reset();
		write(address, data);
	}

	void init_read_byte(std::uint32_t address) override
	{
		reset();
		m_latched_byte = read<std::uint8_t>(address);
	}

	void init_read_word(std::uint32_t address) override
	{
		reset();
		m_latched_word = read<std::uint16_t>(address);
	}

	std::uint8_t latched_byte() const override
	{
		if(!m_latched_byte.has_value())
			throw genesis::internal_error();
		return m_latched_byte.value();
	}

	std::uint16_t latched_word() const override
	{
		if(!m_latched_word.has_value())
			throw genesis::internal_error();
		return m_latched_word.value();
	}

	/* direct interface */

	template<class T>
	void write(std::uint32_t address, T data)
	{
		check_addr(address, sizeof(T));

		// convert to proper byte order
		if (m_byte_order == std::endian::little)
		{
			endian::sys_to_little(data);
		}
		else if (m_byte_order == std::endian::big)
		{
			endian::sys_to_big(data);
		}

		// TODO: why do we need to iterate?
		for (size_t i = 0; i < sizeof(T); ++i)
			m_buffer[address + i] = *(reinterpret_cast<std::uint8_t*>(&data) + i);
	}

	template<class T>
	T read(std::uint32_t address)
	{
		check_addr(address, sizeof(T));

		T data = *reinterpret_cast<T*>(&(m_buffer[address]));

		// convert to sys byte order
		if (m_byte_order == std::endian::little)
		{
			endian::little_to_sys(data);
		}
		else if (m_byte_order == std::endian::big)
		{
			endian::big_to_sys(data);
		}

		return data;
	}

	/* Read data without BE/LE conversion */
	template<class T>
	T read_raw(std::uint32_t address)
	{
		check_addr(address, sizeof(T));
		T data = *reinterpret_cast<T*>(&(m_buffer[address]));
		return data;
	}

private:
	inline void check_addr(std::uint32_t addr, size_t size)
	{
		// TODO: change exception type
		if(addr > max_address() || (addr + size - 1) > max_address())
			throw internal_error("buffer_unit check: wrong address (" + su::hex_str(addr) +
									 ") size: " + std::to_string(size));
	}

	void reset()
	{
		m_latched_byte.reset();
		m_latched_word.reset();
	}

private:
	std::span<std::uint8_t> m_buffer;
	std::endian m_byte_order;

	std::optional<std::uint8_t> m_latched_byte;
	std::optional<std::uint16_t> m_latched_word;
};

}

#endif // __buffer_BASE_UNIT_H__
