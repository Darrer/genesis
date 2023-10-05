#include "bus_manager.h"

#include <source_location>


namespace genesis::m68k
{

bus_manager::bus_manager(m68k::cpu_bus& bus, m68k::cpu_registers& regs, exception_manager& exman,
						 std::shared_ptr<memory::addressable> external_memory)
	: bus(bus), regs(regs), exman(exman), external_memory(external_memory)
{
	state = bus_cycle_state::IDLE;
}

void bus_manager::reset()
{
	on_complete_cb = nullptr;
	modify_cb = nullptr;
	state = bus_cycle_state::IDLE;
	clear_bus();
}

bool bus_manager::is_idle() const
{
	return state == bus_cycle_state::IDLE;
}

std::uint8_t bus_manager::latched_byte() const
{
	assert_idle();
	if(!byte_operation)
		throw std::runtime_error("bus_manager::latched_byte error: don't have latched byte");

	return external_memory->latched_byte();
}

std::uint16_t bus_manager::latched_word() const
{
	assert_idle();
	if(byte_operation)
		throw std::runtime_error("bus_manager::latched_word error: don't have latched word");

	return external_memory->latched_word();
}

/* bus control interface */

bool bus_manager::bus_granted() const
{
	// NOTE: BG is set after access has been granted
	return bus.is_set(bus::BG);
}

void bus_manager::request_bus()
{
	if(bus_granted() || bus.is_set(bus::BR))
	{
		// alrady granted or requested, caller shouldn't request it again
		throw internal_error();
	}

	bus.set(bus::BR);
}

void bus_manager::release_bus()
{
	assert_idle();

	if(!bus_granted() || !bus.is_set(bus::BR))
	{
		// In theory, it should be fine to request and release the bus even if it wasn't granted
		// but in practice this is more likly to indicate a misuse (why requsted and not used?),
		// so rise internal_error exception
		throw internal_error();
	}

	bus.clear(bus::BR);
}

void bus_manager::assert_idle(std::source_location loc) const
{
	if(!is_idle())
	{
		throw std::runtime_error(std::string(loc.function_name()) +
								 " error: cannot perform an operation while busy");
	}
}

void bus_manager::cycle()
{
	using enum bus_cycle_state;
	switch(state)
	{
	case IDLE:
		on_idle();
		return;

	/* bus ready cycle */
	case READ0:
	case RMW_READ0:
		if(check_exceptions())
			return;

		bus.func_codes(gen_func_codes());
		bus.set(bus::RW);
		bus.address(address);

		advance_state();
		return;

	case READ1:
	case RMW_READ1:
		bus.set(bus::AS);
		set_data_strobe_bus();

		advance_state();
		return;

	case READ2:
	case RMW_READ2:
		if(byte_operation)
			external_memory->init_read_byte(bus.address());
		else
			external_memory->init_read_word(bus.address());

		advance_state();
		[[fallthrough]];

	case READ_WAIT:
	case RMW_READ_WAIT:
		if(external_memory->is_idle())
		{
			if(byte_operation)
				set_data_bus(external_memory->latched_byte());
			else
				set_data_bus(external_memory->latched_word());

			bus.set(bus::DTACK);
			advance_state();
		}
		return;

	case READ3:
		clear_bus();
		set_idle();
		return;

	/* bus read-modify-write cycles */
	case RMW_READ3:
		clear_bus();
		bus.set(bus::AS); // keep it on

		advance_state();
		return;

	case RMW_MODIFY0:
		// IDLE CYCLE
		advance_state();
		return;

	case RMW_MODIFY1:
		data_to_write = std::uint8_t(modify_cb(external_memory->latched_byte()));
		advance_state();
		return;

	/* bus write cycle */
	case WRITE0:
	case RMW_WRITE0:
		if(check_exceptions())
			return;
		bus.func_codes(gen_func_codes());
		bus.set(bus::RW);
		bus.address(address);

		advance_state();
		return;

	case WRITE1:
	case RMW_WRITE1:
		bus.set(bus::AS);
		bus.clear(bus::RW);
		set_data_bus(data_to_write);

		advance_state();
		return;

	case WRITE2:
	case RMW_WRITE2:
		set_data_strobe_bus();

		if(byte_operation)
			external_memory->init_write(bus.address(), std::uint8_t(data_to_write));
		else
			external_memory->init_write(bus.address(), data_to_write);

		advance_state();
		[[fallthrough]];

	case WRITE_WAIT:
	case RMW_WRITE_WAIT:
		if(external_memory->is_idle())
		{
			bus.set(bus::DTACK);
			advance_state();
		}
		return;

	case WRITE3:
	case RMW_WRITE3:
		clear_bus();
		bus.set(bus::RW);

		set_idle();
		return;

	default:
		throw internal_error();
	}
}

void bus_manager::advance_state()
{
	// should be safe
	auto new_state = static_cast<std::underlying_type_t<bus_cycle_state>>(state) + 1;
	state = static_cast<bus_cycle_state>(new_state);
}

void bus_manager::set_idle()
{
	if(state == bus_cycle_state::IDLE)
		return;

	state = bus_cycle_state::IDLE;

	if(on_complete_cb)
		on_complete_cb();
	on_complete_cb = nullptr;

	on_idle();

	// If callback started a new operation, throw an exception
	// chaining leads to occupying bus for 2 (or more) bus cycles,
	// we should not allow that
	assert_idle();
}

void bus_manager::on_idle()
{
	// TODO: with the current approach sometimes it takes 1 cycle to grant bus,
	// but sometimes access is granted right after finishing current bus operation
	// it seems we have to unify this behavior
	if(bus.is_set(bus::BR) && !bus_granted())
	{
		// we're idle and bus is requsted - perfect time to give it up
		bus.set(bus::BG); // grant access just by setting BR flag
	} 
	else if(bus_granted() && !bus.is_set(bus::BR))
	{
		// we can become master again
		bus.clear(bus::BG);
	}
}

/* bus helpers */

void bus_manager::clear_bus()
{
	bus.clear(bus::AS);
	bus.clear(bus::UDS);
	bus.clear(bus::LDS);
	bus.clear(bus::DTACK);
	bus.clear(bus::FC0);
	bus.clear(bus::FC1);
	bus.clear(bus::FC2);
	bus.clear(bus::BERR);
	// NOTE: clear_bus cannot clear BR/BG buses as it's usualy called in the end of a bus cycle
	// therefore by clearing BR/BG we can clear current state
}

std::uint8_t bus_manager::gen_func_codes() const
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

void bus_manager::set_data_strobe_bus()
{
	if(byte_operation)
	{
		auto ds = address_even ? bus::UDS : bus::LDS;
		bus.set(ds);
	}
	else
	{
		bus.set(bus::LDS);
		bus.set(bus::UDS);
	}
}

void bus_manager::set_data_bus(std::uint16_t data)
{
	if(byte_operation)
	{
		if(address_even) // uds
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

/* exceptoins */

bool bus_manager::check_exceptions()
{
	return check_address_error() || check_bus_error();
}

bool bus_manager::check_bus_error()
{
	// TODO: temporarily disabled
	if(false && should_rise_bus_error())
	{
		rise_bus_error();
		return true;
	}

	return false;
}

bool bus_manager::should_rise_bus_error() const
{
	const std::uint32_t max_address = external_memory->max_address();
	auto size = byte_operation ? 1 : 2;
	// TODO: check only 24 bits (as only they are put on the address bus)
	return (address & max_address) + size > max_address;
}

void bus_manager::rise_bus_error()
{
	// TODO: what if bus is granted to some other device?
	// TODO: check for RMW_READ0
	bool read_operation = state == bus_cycle_state::READ0;
	exman.rise_bus_error({address, gen_func_codes(), read_operation, false});
	reset();
}

bool bus_manager::check_address_error()
{
	if(should_rise_address_error())
	{
		rise_address_error();
		return true;
	}

	return false;
}

bool bus_manager::should_rise_address_error() const
{
	return !byte_operation && !address_even;
}

void bus_manager::rise_address_error()
{
	// TODO: what if bus is granted to some other device?

	// NOTE: we don't need to check for RMW_READ0 state as read-modify-write cycle performs
	// only single byte operations and they cannot generate address error exception.
	bool read_operation = state == bus_cycle_state::READ0;
	bool in = space == addr_space::PROGRAM; // just to satisfy external tests
	exman.rise_address_error({address, gen_func_codes(), read_operation, in});
	reset();
}

} // namespace genesis::m68k
