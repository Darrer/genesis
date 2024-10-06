#ifndef __M68K_BUS_MANAGER_H__
#define __M68K_BUS_MANAGER_H__

#include "exception_manager.h"
#include "m68k/cpu_bus.hpp"
#include "m68k/cpu_registers.hpp"
#include "m68k/interrupting_device.h"
#include "memory/addressable.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>


namespace genesis::m68k
{

enum class addr_space
{
	PROGRAM,
	DATA,
	CPU,
};

// NOTE: we can optimize bus_manager by specifying single on_complete call back instance (not per operation)
class bus_manager
{
private:
	enum class bus_cycle_state
	{
		IDLE,

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

		/* bus interrupt acknowledge cycle */
		IAC0,
		IAC1,
		IAC2,
		IAC_WAIT,
		IAC3,
	};


public:
	using on_complete = std::function<void()>;
	using on_modify = std::function<std::uint8_t(std::uint8_t)>;

private:
	// all callbacks are restricted in size to the size of a pointer
	// this is required for std::function small-size optimizations
	// (though it's not guaranteed by standard, so we purely rely on implementation)
	constexpr const static std::size_t max_callable_size = sizeof(void*);

public:
	bus_manager(m68k::cpu_bus& bus, m68k::cpu_registers& regs, exception_manager& exman,
				std::shared_ptr<memory::addressable> external_memory, std::shared_ptr<interrupting_device> int_dev);

	void cycle();

	void reset();
	bool is_idle() const;

	/* read/write interface */

	template <class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint8_t data, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		start_new_operation(address, addr_space::DATA, bus_cycle_state::WRITE0, cb);
		data_to_write = data;
		byte_operation = true;
	}

	template <class Callable = std::nullptr_t>
	void init_write(std::uint32_t address, std::uint16_t data, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		start_new_operation(address, addr_space::DATA, bus_cycle_state::WRITE0, cb);
		data_to_write = data;
		byte_operation = false;
	}

	template <class OnModify, class OnComplete = std::nullptr_t>
	void init_read_modify_write(std::uint32_t address, OnModify modify, addr_space space, OnComplete cb = nullptr)
	{
		static_assert(sizeof(OnModify) <= max_callable_size);
		static_assert(sizeof(OnComplete) <= max_callable_size);
		assert_idle();

		modify_cb = modify;
		if(modify_cb == nullptr)
			throw std::invalid_argument("modify");

		start_new_operation(address, space, bus_cycle_state::RMW_READ0, cb);
		byte_operation = true;
	}

	template <class Callable = std::nullptr_t>
	void init_read_modify_write(std::uint32_t address, on_modify&& modify, addr_space space, Callable cb)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		modify_cb = std::move(modify);
		start_new_operation(address, space, bus_cycle_state::RMW_READ0, cb);
		byte_operation = true;
	}

	template <class Callable = std::nullptr_t>
	void init_read_byte(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		start_new_operation(address, space, bus_cycle_state::READ0, cb);
		byte_operation = true;
	}

	template <class Callable = std::nullptr_t>
	void init_read_word(std::uint32_t address, addr_space space, Callable cb = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		start_new_operation(address, space, bus_cycle_state::READ0, cb);
		byte_operation = false;
	}

	std::uint8_t latched_byte() const;
	std::uint16_t latched_word() const;

	/* bus arbitration interface */

	bool bus_granted() const;
	void request_bus();
	void release_bus();

	/* interrupt interface */

	template <class Callable = std::nullptr_t>
	void init_interrupt_ack(std::uint8_t ipl, Callable on_complete = nullptr)
	{
		static_assert(sizeof(Callable) <= max_callable_size);
		assert_idle();

		if(ipl == 0 || ipl > 7)
			throw genesis::internal_error();

		state = bus_cycle_state::IAC0;
		m_ipl = ipl;
		vector_number.reset();
		on_complete_cb = on_complete;
	}

	std::uint8_t get_vector_number() const;

private:
	void assert_idle(std::source_location loc = std::source_location::current()) const;

	template <class Callable = std::nullptr_t>
	void start_new_operation(std::uint32_t addr, addr_space sp, bus_cycle_state first_state, Callable cb = nullptr)
	{
		address = addr;
		address_even = (address & 1) == 0;
		space = sp;
		state = first_state;
		on_complete_cb = cb;
		vector_number.reset();
	}

	void advance_state();

	void set_idle();
	void on_idle();

	/* bus helpers */
	void clear_bus();
	std::uint8_t gen_func_codes() const;
	std::uint32_t gen_int_addr() const;
	void set_data_strobe_bus();
	void set_data_bus(std::uint16_t data);

	/* exceptoins */
	bool check_exceptions();

	bool should_rise_bus_error() const;
	void rise_bus_error();

	bool should_rise_address_error() const;
	void rise_address_error();

private:
	m68k::cpu_bus& bus;
	m68k::cpu_registers& regs;
	m68k::exception_manager& exman;
	std::shared_ptr<memory::addressable> external_memory;
	std::shared_ptr<interrupting_device> int_dev;

	on_complete on_complete_cb = nullptr;
	on_modify modify_cb = nullptr;

	bus_cycle_state state;

	bool byte_operation;
	std::uint32_t address;
	bool address_even; // TODO: do we need it?
	addr_space space;
	std::uint16_t data_to_write;
	std::uint8_t m_ipl;
	std::optional<std::uint8_t> vector_number;
};

}; // namespace genesis::m68k

#endif // __M68K_BUS_MANAGER_H__
