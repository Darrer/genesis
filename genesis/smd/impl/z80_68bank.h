#ifndef __SMD_IMPL_Z80_68BANK_H__
#define __SMD_IMPL_Z80_68BANK_H__

#include <memory>
#include <stdexcept>

#include "memory/addressable.h"
#include "memory/memory_unit.h"
#include "exception.hpp"

#include <iostream>
#include "string_utils.hpp"

namespace genesis::impl
{

class bank_register : public memory::addressable
{
public:
	/* addressable interface */
	std::uint32_t max_address() const override
	{
		return 0x1;
	}

	bool is_idle() const
	{
		return true;
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		throw std::runtime_error("z80 bank register does not support 16-bit write operations");
	}

	void init_read_word(std::uint32_t address) override
	{
		throw std::runtime_error("z80 bank register does not support 16-bit read operations");
	}

	std::uint16_t latched_word() const override
	{
		throw genesis::internal_error();
	}

	void init_read_byte(std::uint32_t /* address */) override
	{
	}

	std::uint8_t latched_byte() const override
	{
		// reading always returns 0xFF
		return 0xFF;
	}

	void init_write(std::uint32_t /* address */, std::uint8_t data) override
	{
		// only LSB is written
		data = data & 0b1;

		// alwyas write MSB first
		m_bank_register = (m_bank_register >> 1) | (data << 8);

		std::cout << "Updating bank register: " << su::bin_str(m_bank_register) << ", data: " << (int)data << '\n';
	}

	std::uint32_t bank_value() const
	{
		return m_bank_register;
	}

private:
	std::uint32_t m_bank_register = 0;
};

class bank_area : public memory::addressable
{
public:
	bank_area(std::shared_ptr<bank_register> bank_register, std::shared_ptr<memory::addressable> m68k_area)
		: m_bank_register(bank_register), m_memory(m68k_area)
	{
	}

	std::uint32_t max_address() const override
	{
		// 0xFFFF - 0x8000
		return 0x7FFF;
	}

	bool is_idle() const override
	{
		return true;
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		m_memory->init_write(fix_address(address), data);
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		m_memory->init_write(fix_address(address), data);
	}

	void init_read_byte(std::uint32_t address) override
	{
		m_memory->init_read_byte(fix_address(address));
	}

	void init_read_word(std::uint32_t address) override
	{
		m_memory->init_read_word(fix_address(address));
	}

	std::uint8_t latched_byte() const override
	{
		try
		{
			return m_memory->latched_byte();
		}
		catch(const std::exception&)
		{
			throw genesis::internal_error();
		}
	}

	std::uint16_t latched_word() const override
	{
		try
		{
			return m_memory->latched_word();
		}
		catch(const std::exception&)
		{
			throw genesis::internal_error();
		}
	}

private:
	std::uint32_t fix_address(std::uint32_t address)
	{
		std::uint32_t bank_reg = m_bank_register->bank_value();
		bank_reg = bank_reg << 15;

		address = address & 0xFFFF;

		address = bank_reg | address;

		if(address > 0x3FFFFF)
			throw not_implemented("z80 bank area: only ROM is supported for now (address: "
				+ su::hex_str(address) + ")");

		return address;
	}

private:
	std::shared_ptr<memory::addressable> m_memory;
	std::shared_ptr<bank_register> m_bank_register;
};

class z80_68bank
{
public:
	z80_68bank(std::shared_ptr<memory::addressable> m68k_memory)
	{
		auto bank_reg = std::make_shared<impl::bank_register>();
		m_bank_reg = bank_reg;

		m_bank_area = std::make_shared<impl::bank_area>(bank_reg, m68k_memory);
	}

	std::shared_ptr<memory::addressable> bank_register()
	{
		return m_bank_reg;
	}

	std::shared_ptr<memory::addressable> bank_area()
	{
		return m_bank_area;
	}

private:
	std::shared_ptr<memory::addressable> m_bank_reg;
	std::shared_ptr<memory::addressable> m_bank_area;
};

}

#endif // __SMD_IMPL_Z80_68BANK_H__
