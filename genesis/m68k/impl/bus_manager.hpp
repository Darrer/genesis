#ifndef __M68K_BUS_MANAGER_HPP__
#define __M68K_BUS_MANAGER_HPP__

#include <cstdint>
#include <optional>
#include <functional>
#include <limits>

#include "m68k/cpu_memory.h"
#include "m68k/cpu_bus.hpp"
#include "m68k/cpu_registers.hpp"

#include "exception_manager.h"

namespace genesis::m68k
{

enum class addr_space : std::uint8_t
{
	PROGRAM,
	DATA,
	CPU,
};

class bus_manager
{
public:
	using on_complete = std::function<void()>;

private:
	enum bus_cycle_state : std::uint8_t
	{
		IDLE,

		/* bus ready cycle */
		READ0,
		READ1,
		READ2,
		READ3,

		/* bus write cycle */
		WRITE0,
		WRITE1,
		WRITE2,
		WRITE3,
	};

public:
	bus_manager(m68k::cpu_bus& bus, m68k::cpu_registers& regs, m68k::memory& mem, exception_manager& exman)
		: bus(bus), regs(regs), mem(mem), exman(exman) { }

	bool is_idle() const
	{
		return state == bus_cycle_state::IDLE;
	}

	std::uint8_t letched_byte() const
	{
		assert_idle("letched_byte");

		if(!_letched_byte.has_value())
			throw std::runtime_error("bus_manager::letched_byte error: don't have letched byte");
		
		return _letched_byte.value();
	}

	std::uint16_t letched_word() const
	{
		assert_idle("letched_word");

		if(!_letched_word.has_value())
			throw std::runtime_error("bus_manager::letched_word error: don't have letched word");

		return _letched_word.value();
	}

	void init_read_byte(std::uint32_t address, addr_space _space, on_complete cb = nullptr)
	{
		assert_idle("init_read_byte");

		init_new_cycle(address, READ0, cb);
		byte_op = true;
		space = _space;
	}

	void init_read_word(std::uint32_t address, addr_space _space, on_complete cb = nullptr)
	{
		assert_idle("init_read_word");

		init_new_cycle(address, READ0, cb);
		byte_op = false;
		space = _space;
	}

	template<class T>
	void init_write(std::uint32_t address, T data, on_complete cb = nullptr)
	{
		static_assert(sizeof(T) <= 2);
		static_assert(std::numeric_limits<T>::is_signed == false);

		assert_idle("init_write");

		init_new_cycle(address, WRITE0, cb);
		data_to_write = data;
		byte_op = sizeof(T) == 1;
		space = addr_space::DATA; // TODO: can write be to PROGRAM space?
	}

	void cycle()
	{
		// using bus_cycle_state;
		switch (state)
		{
		case IDLE:
			break;

		/* bus ready cycle */
		case READ0:
			// check for address error
			if(should_rise_address_error())
			{
				rise_address_error();
				break;
			}

			set_func_codes();
			bus.set(bus::RW);
			bus.address(address);
			state = READ1; break;

		case READ1:
			bus.set(bus::AS);
			set_data_strobe_bus();
			state = READ2; break;

		case READ2:
		{
			// emulate read
			std::uint16_t data = 0;
			if(byte_op)
				data = mem.read<std::uint8_t>(bus.address());
			else
				data = mem.read<std::uint16_t>(bus.address());
			set_data_bus(data);

			// immidiate DTACK
			bus.set(bus::DTACK);
		}
			state = READ3; break;

		case READ3:
			if(byte_op)
			{
				if(bus.is_set(bus::LDS))
					_letched_byte = bus.data() & 0xFF;
				else
					_letched_byte = bus.data() >> 8;
			}
			else
			{
				_letched_word = bus.data();
			}
			clear_bus();
			set_idle(); break;

		/* bus write cycle */
		case WRITE0:
			set_func_codes();
			bus.set(bus::RW);
			bus.address(address);
			state = WRITE1; break;

		case WRITE1:
			bus.set(bus::AS);
			bus.clear(bus::RW);
			set_data_bus(data_to_write);
			state = WRITE2; break;

		case WRITE2:
			set_data_strobe_bus();

			// emulate write
			if(byte_op)
				mem.write<std::uint8_t>(bus.address(), data_to_write & 0xFF);
			else
				mem.write<std::uint16_t>(bus.address(), data_to_write);

			// immidiate DTACK
			bus.set(bus::DTACK);
			state = WRITE3; break;

		case WRITE3:
			clear_bus();
			bus.set(bus::RW);
			set_idle(); break;

		default:
			throw std::runtime_error("bus_manager::cycle internal error: unknown state");
		}
	}

private:
	void init_new_cycle(std::uint32_t addr, bus_cycle_state first_state, on_complete cb)
	{
		reset();

		address = addr;
		state = first_state;
		on_complete_cb = cb;
	}

	void assert_idle(std::string_view caller) const
	{
		if(!is_idle())
		{
			throw std::runtime_error("bus_manager::" + std::string(caller) +
				" error: cannot perform the operation while in the middle of cycle");
		}
	}

	void set_idle()
	{
		if(state == IDLE)
			return;

		state = IDLE;

		auto cb = on_complete_cb; // to allow chaining
		on_complete_cb = nullptr; // to prevent extra calls

		if(cb) cb();
	}

	void set_data_strobe_bus()
	{
		if(byte_op)
		{
			auto ds = (address & 1) == 0 ? bus::UDS : bus::LDS;
			bus.set(ds);
		}
		else
		{
			bus.set(bus::LDS);
			bus.set(bus::UDS);
		}
	}

	void set_data_bus(std::uint16_t data)
	{
		if(byte_op)
		{
			bool uds = (address & 1) == 0;
			if(uds)
			{
				data = data << 8;
				data = data | (bus.data() & 0xFF);
			}
			else // lds
			{
				data = data & 0xFF;
				data = (bus.data() & 0xFF00) | data;
			}
		}

		bus.data(data);
	}

	std::uint8_t gen_func_codes() const
	{
		std::uint8_t func_codes = 0;
		if(space == addr_space::DATA)
			func_codes |= 1;
		if(space == addr_space::PROGRAM)
			func_codes |= 1 << 1;
		if(regs.flags.S)
			func_codes |= 1 << 2;
		return func_codes;
	}

	void set_func_codes()
	{
		clear_funcs_codes();

		if(regs.flags.S)
			bus.set(bus::FC2);
		if(space == addr_space::PROGRAM)
			bus.set(bus::FC1);
		if(space == addr_space::DATA)
			bus.set(bus::FC0);
	}

	void clear_funcs_codes()
	{
		bus.clear(bus::FC0);
		bus.clear(bus::FC1);
		bus.clear(bus::FC2);
	}

	bool should_rise_address_error() const
	{
		return !byte_op && (address % 2) == 1;
	}

	void rise_address_error()
	{
		std::uint8_t func_codes = gen_func_codes();
		// if(bus.is_set(bus::FC2))
		// 	func_codes |= 0b100;
		// if(bus.is_set(bus::FC1))
		// 	func_codes |= 0b010;
		// if(bus.is_set(bus::FC0))
		// 	func_codes |= 0b001;

		exman.rise_address_error( { address, regs.PC - 2, func_codes, true, false } );
		reset();
	}

	void reset()
	{
		on_complete_cb = nullptr;
		data_to_write = 0;
		_letched_byte.reset();
		_letched_word.reset();
		state = bus_cycle_state::IDLE;
		clear_bus();
	}

	void clear_bus()
	{
		bus.clear(bus::AS);
		bus.clear(bus::UDS);
		bus.clear(bus::LDS);
		bus.clear(bus::DTACK);
		clear_funcs_codes();
	}

private:
	m68k::cpu_bus& bus;
	m68k::cpu_registers& regs;
	m68k::memory& mem;
	exception_manager& exman;

	std::uint32_t address;
	addr_space space;
	std::uint16_t data_to_write;
	on_complete on_complete_cb = nullptr;

	bool byte_op = false;
	std::optional<std::uint8_t> _letched_byte;
	std::optional<std::uint16_t> _letched_word;

	bus_cycle_state state = bus_cycle_state::IDLE;
};

}

#endif // __M68K_BUS_MANAGER_HPP__
