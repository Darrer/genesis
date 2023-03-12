#ifndef __M68K_BUS_MANAGER_HPP__
#define __M68K_BUS_MANAGER_HPP__

#include <cstdint>
#include <optional>
#include <functional>
#include <limits>

#include "m68k/cpu_memory.h"
#include "m68k/cpu_bus.hpp"


namespace genesis::m68k
{

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
	bus_manager(m68k::cpu_bus& bus, m68k::memory& mem) : bus(bus), mem(mem) { }

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

	void init_read_byte(std::uint32_t address, on_complete cb = nullptr)
	{
		assert_idle("init_read_byte");

		init_new_cycle(address, READ0, cb);
		byte_op = true;
	}

	void init_read_word(std::uint32_t address, on_complete cb = nullptr)
	{
		assert_idle("init_read_word");

		init_new_cycle(address, READ0, cb);
		byte_op = false;
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
			// TODO: set FC0-FC2
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
			bus.clear(bus::AS);
			bus.clear(bus::LDS);
			bus.clear(bus::UDS);
			bus.clear(bus::DTACK);
			set_idle(); break;

		/* bus write cycle */
		case WRITE0:
			// TODO: set FC0-FC2
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
				mem.write<std::uint8_t>(bus.address(), data_to_write);
			else
				mem.write<std::uint16_t>(bus.address(), data_to_write);

			// immidiate DTACK
			bus.set(bus::DTACK);
			state = WRITE3; break;

		case WRITE3:
			bus.clear(bus::AS);
			bus.clear(bus::UDS);
			bus.clear(bus::LDS);
			bus.set(bus::RW);
			bus.clear(bus::DTACK);
			set_idle(); break;

		default:
			throw std::runtime_error("bus_manager::cycle internal error: unknown state");
		}
	}

private:
	void init_new_cycle(std::uint32_t addr, bus_cycle_state first_state, on_complete cb)
	{
		address = addr;
		state = first_state;
		on_complete_cb = cb;

		// reset
		data_to_write = 0;
		byte_op = false;
		_letched_byte.reset();
		_letched_word.reset();
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

		if(on_complete_cb)
			on_complete_cb();
		on_complete_cb = nullptr; // to prevent extra calls
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

private:
	m68k::cpu_bus& bus;
	m68k::memory& mem;

	std::uint32_t address;
	std::uint16_t data_to_write;
	on_complete on_complete_cb = nullptr;

	bool byte_op = false;
	std::optional<std::uint8_t> _letched_byte;
	std::optional<std::uint16_t> _letched_word;

	bus_cycle_state state = bus_cycle_state::IDLE;
};

}

#endif // __M68K_BUS_MANAGER_HPP__
