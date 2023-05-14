#ifndef __M68K_BUS_MANAGER_HPP__
#define __M68K_BUS_MANAGER_HPP__

#include <cstdint>
#include <optional>
#include <functional>

#include "m68k/cpu_memory.h"
#include "m68k/cpu_bus.hpp"
#include "m68k/cpu_registers.hpp"

#include "exception_manager.h"


namespace genesis::m68k
{

enum class addr_space
{
	PROGRAM,
	DATA,
	CPU,
};

class bus_manager
{
public:
	using on_complete = std::function<void()>;
	using on_modify = std::function<std::uint8_t(std::uint8_t)>;

private:
	// all callbacks are restricted in size to the size of the pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

	enum bus_cycle_state
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

		/* bus read modify write cycle */

		// Read
		RMW_READ0,
		RMW_READ1,
		RMW_READ2,
		RMW_READ3,

		// Modify
		RMW_MODIFY0,
		RMW_MODIFY1,

		// Write
		RMW_WRITE0,
		RMW_WRITE1,
		RMW_WRITE2,
		RMW_WRITE3,
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

	template<class Callable = std::nullptr_t>
	void init_read_byte(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_byte");

		init_new_cycle(address, READ0, cb);
		byte_op = true;
		this->space = space;
	}

	template<class Callable = std::nullptr_t>
	void init_read_word(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_word");

		init_new_cycle(address, READ0, cb);
		byte_op = false;
		this->space = space;
	}

	template<class T, class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, T data, Callable cb = nullptr)
	{
		static_assert(sizeof(T) <= 2);
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_write");

		init_new_cycle(address, WRITE0, cb);
		data_to_write = data;
		byte_op = sizeof(T) == 1;
		space = addr_space::DATA;
	}

	template<class Callable>
	void init_read_modify_write(std::uint32_t address, Callable modify, addr_space space = addr_space::DATA)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_modify_write");

		reset();

		modify_cb = modify;
		if(modify_cb == nullptr)
			throw std::invalid_argument("modify");

		this->address = address;
		this->space = space;
		state = RMW_READ0;
		byte_op = true;
	}

	void cycle()
	{
		switch (state)
		{
		case IDLE:
			break;

		case READ0:
			if(check_exceptions())
				return;
			do_state(state);
			state = READ1;
			break;
		case READ1:
			do_state(state);
			state = READ2;
			break;
		case READ2:
			do_state(state);
			state = READ3;
			break;
		case READ3:
			do_state(state);
			set_idle();
			break;

		case WRITE0:
			if(check_exceptions())
				return;
			do_state(state);
			state = WRITE1;
			break;
		case WRITE1:
			do_state(state);
			state = WRITE2;
			break;
		case WRITE2:
			do_state(state);
			state = WRITE3;
			break;
		case WRITE3:
			do_state(state);
			set_idle();
			break;

		case RMW_READ0:
			do_state(READ0);
			state = RMW_READ1;
			break;
		case RMW_READ1:
			do_state(READ1);
			state = RMW_READ2;
			break;
		case RMW_READ2:
			do_state(READ2);
			state = RMW_READ3;
			break;
		case RMW_READ3:
			do_state(READ3);
			bus.set(bus::AS); // keep it on
			state = RMW_MODIFY0;
			break;

		case RMW_MODIFY0:
			state = RMW_MODIFY1;
			break;
		case RMW_MODIFY1:
			data_to_write = modify_cb(_letched_byte.value());
			state = RMW_WRITE0;
			break;

		case RMW_WRITE0:
			do_state(WRITE0);
			state = RMW_WRITE1;
			break;
		case RMW_WRITE1:
			do_state(WRITE1);
			state = RMW_WRITE2;
			break;
		case RMW_WRITE2:
			do_state(WRITE2);
			state = RMW_WRITE3;
			break;
		case RMW_WRITE3:
			do_state(WRITE3);
			set_idle();
			break;

		default: throw internal_error();
		}
	}

private:
	void do_state(bus_cycle_state state)
	{
		switch(state)
		{
		/* bus ready cycle */
		case READ0:
			bus.func_codes(gen_func_codes());
			bus.set(bus::RW);
			bus.address(address);
			return;

		case READ1:
			bus.set(bus::AS);
			set_data_strobe_bus();
			return;

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
			return;
		}

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
			return;

		/* bus write cycle */
		case WRITE0:
			bus.func_codes(gen_func_codes());
			bus.set(bus::RW);
			bus.address(address);
			return;

		case WRITE1:
			bus.set(bus::AS);
			bus.clear(bus::RW);
			set_data_bus(data_to_write);
			return;

		case WRITE2:
			set_data_strobe_bus();

			// emulate write
			if(byte_op)
				mem.write<std::uint8_t>(bus.address(), data_to_write & 0xFF);
			else
				mem.write<std::uint16_t>(bus.address(), data_to_write);

			// immidiate DTACK
			bus.set(bus::DTACK);
			return;

		case WRITE3:
			clear_bus();
			bus.set(bus::RW);
			return;

		default: throw internal_error();
		}
	}

private:
	void init_new_cycle(std::uint32_t addr, bus_cycle_state first_state, on_complete cb)
	{
		_letched_byte.reset();
		_letched_word.reset();

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

		// TODO: do not allow chaining
		if(on_complete_cb)
			on_complete_cb();
		on_complete_cb = nullptr;
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
		bus.clear(bus::FC0);
		bus.clear(bus::FC1);
		bus.clear(bus::FC2);
	}

	// exceptions
	bool check_exceptions()
	{
		return check_address_error() || check_bus_error();
	}

	bool check_bus_error()
	{
		if(false && should_rise_bus_error())
		{
			rise_bus_error();
			return true;
		}

		return false;
	}

	bool should_rise_bus_error() const
	{
		const std::uint32_t max_address = 0xFFFFFF;
		std::uint8_t size = byte_op ? 1 : 2;
		return (address & max_address) + size > max_address;
	}

	void rise_bus_error()
	{
		std::uint8_t func_codes = gen_func_codes();
		bool read_operation = state == bus_cycle_state::READ0;
		exman.rise_bus_error( { address, func_codes, read_operation, false } );
		reset();
	}

	bool check_address_error()
	{
		if(should_rise_address_error())
		{
			rise_address_error();
			return true;
		}

		return false;
	}

	bool should_rise_address_error() const
	{
		return !byte_op && (address % 2) == 1;
	}

	void rise_address_error()
	{
		std::uint8_t func_codes = gen_func_codes();
		bool read_operation = state == bus_cycle_state::READ0;
		exman.rise_address_error( { address, func_codes, read_operation, false } );
		reset();
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
	on_modify modify_cb = nullptr;

	bool byte_op = false;
	std::optional<std::uint8_t> _letched_byte;
	std::optional<std::uint16_t> _letched_word;

	bus_cycle_state state = bus_cycle_state::IDLE;
};

}

#endif // __M68K_BUS_MANAGER_HPP__
