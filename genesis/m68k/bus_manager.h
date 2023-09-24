#ifndef __M68K_BUS_MANAGER_H__
#define __M68K_BUS_MANAGER_H__

#include <cstdint>
#include <functional>
#include <memory>

#include "memory/addressable.h"
#include "cpu_bus.hpp"
#include "cpu_registers.hpp"
#include "impl/exception_manager.h"


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
private:
	enum bus_cycle_state
	{
		IDLE,

		WAIT,

		/* bus ready cycle */
		READ0,
		READ1,
		READ2,
		READ_WAIT,
		READ3,

		/* bus write cycle */
		WRITE0,
		WRITE1,
		WRITE2,
		WRITE_WAIT,
		WRITE3,

		/* bus read modify write cycle */

		// Read
		RMW_READ0,
		RMW_READ1,
		RMW_READ2,
		RMW_READ_WAIT,
		RMW_READ3,

		// Modify
		RMW_MODIFY0,
		RMW_MODIFY1,

		// Write
		RMW_WRITE0,
		RMW_WRITE1,
		RMW_WRITE2,
		RMW_WRITE_WAIT,
		RMW_WRITE3,
	};


public:
	using on_complete = std::function<void()>;
	using on_modify = std::function<std::uint_fast8_t(std::uint_fast8_t)>;

private:
	// all callbacks are restricted in size to the size of a pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

public:
	bus_manager(m68k::cpu_bus& bus, m68k::cpu_registers& regs, exception_manager& exman,
		std::shared_ptr<memory::addressable> external_memory);

	void cycle(); // TODO: must be visible only for m68k::cpu

	void reset();
	bool is_idle() const;

	/* read/write interface */

	template<class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint8_t data, const Callable& cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_write byte");

		start_new_operation(address, addr_space::DATA, WRITE0, cb);
		data_to_write = data;
		byte_operation = true;
	}

	template<class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint16_t data, const Callable& cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_write word");

		start_new_operation(address, addr_space::DATA, WRITE0, cb);
		data_to_write = data;
		byte_operation = false;
	}

	template<class Callable>
	void init_read_modify_write(std::uint32_t address, const Callable& modify, addr_space space = addr_space::DATA)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_modify_write");

		modify_cb = modify;
		if(modify_cb == nullptr)
			throw std::invalid_argument("modify");

		this->address = address;
		this->address_even = (address & 1) == 0;
		this->space = space;
		state = RMW_READ0;
		byte_operation = true;
	}

	template<class Callable = std::nullptr_t>
	void init_read_byte(std::uint32_t address, addr_space space, const Callable& cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_byte");

		start_new_operation(address, space, READ0, cb);
		byte_operation = true;
	}

	template<class Callable = std::nullptr_t>
	void init_read_word(std::uint32_t address, addr_space space, const Callable& cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle("init_read_word");

		start_new_operation(address, space, READ0, cb);
		byte_operation = false;
	}

	std::uint8_t latched_byte() const;
	std::uint16_t latched_word() const;

	/* bus control interface */

	/* interrupt interface */

private:
	void assert_idle(std::string_view caller) const;

	template<class Callable = std::nullptr_t>
	void start_new_operation(std::uint32_t addr, addr_space sp, int first_state, const Callable& cb)
	{
		address = addr;
		address_even = (address & 1) == 0;
		space = sp;
		state = first_state;
		on_complete_cb = cb;
	}

	void do_state(int state);
	void advance_state();

	void set_idle();

	/* bus helpers */
	void clear_bus();
	std::uint8_t gen_func_codes() const;
	void set_data_strobe_bus();
	void set_data_bus(std::uint16_t data);

	/* exceptoins */
	bool check_exceptions();

	bool check_bus_error();
	bool should_rise_bus_error() const;
	void rise_bus_error();

	bool check_address_error();
	bool should_rise_address_error() const;
	void rise_address_error();

private:
	m68k::cpu_bus& bus;
	m68k::cpu_registers& regs;
	m68k::exception_manager& exman;
	std::shared_ptr<memory::addressable> external_memory;

	on_complete on_complete_cb = nullptr;
	on_modify modify_cb = nullptr;

	int state;

	bool byte_operation;
	std::uint32_t address;
	bool address_even;
	addr_space space;
	std::uint16_t data_to_write;
	std::uint16_t _latched_data;
};

};

#endif // __M68K_BUS_MANAGER_H__
