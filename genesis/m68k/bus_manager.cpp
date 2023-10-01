#include "bus_manager.h"


namespace genesis::m68k
{

bus_manager::bus_manager(m68k::cpu_bus& bus, m68k::cpu_registers& regs, exception_manager& exman,
						 std::shared_ptr<memory::addressable> external_memory)
	: bus(bus), regs(regs), exman(exman), external_memory(external_memory)
{
	state = IDLE;
}

void bus_manager::reset()
{
	on_complete_cb = nullptr;
	modify_cb = nullptr;
	state = IDLE;
	clear_bus();
}

bool bus_manager::is_idle() const
{
	return state == IDLE;
}

std::uint8_t bus_manager::latched_byte() const
{
	assert_idle("latched_byte");
	if (!byte_operation)
		throw std::runtime_error("bus_manager::latched_byte error: don't have latched byte");

	return external_memory->latched_byte();
}

std::uint16_t bus_manager::latched_word() const
{
	assert_idle("latched_word");
	if (byte_operation)
		throw std::runtime_error("bus_manager::latched_word error: don't have latched word");

	return external_memory->latched_word();
}

/* bus control interface */

bool bus_manager::bus_granted() const
{
	// NOTE: BG is set after access has been granted
	return bus.is_set(bus::BG);
}

void bus_manager::request_access()
{
	assert_idle("request_access");

	if (bus_granted() || bus.is_set(bus::BR))
	{
		// alrady granted or requested, caller shouldn't request it again
		throw internal_error();
	}

	bus.set(bus::BR);
}

void bus_manager::release_access()
{
	assert_idle("release_access");

	if (!bus_granted() || !bus.is_set(bus::BR))
	{
		// bus release is requested, but it's not granted nor requested
		throw internal_error();
	}

	bus.clear(bus::BR);
}

void bus_manager::assert_idle(std::string_view caller) const
{
	if (!is_idle())
	{
		throw std::runtime_error("bus_manager::" + std::string(caller) +
								 " error: cannot perform an operation while busy");
	}
}

void bus_manager::cycle()
{
	// TODO: who's gonna execute the requests?
	if (bus_granted())
	{
		if (bus.is_set(bus::BR))
		{
			// the bus is still owned by 3rd device
		}
		else
		{
			// we can become the master again
			bus.clear(bus::BG);
		}

		return;
	}

	switch (state)
	{
	case IDLE:
		do_state(IDLE);
		break;

	case READ0:
		do_state(READ0);
		break;
	case READ1:
		do_state(READ1);
		break;
	case READ2:
		do_state(READ2);
		state = READ_WAIT;
		[[fallthrough]];
	case READ_WAIT:
		do_state(READ_WAIT);
		break;
	case READ3:
		do_state(READ3);
		set_idle();
		break;

	case WRITE0:
		do_state(WRITE0);
		break;
	case WRITE1:
		do_state(WRITE1);
		break;
	case WRITE2:
		do_state(WRITE2);
		state = WRITE_WAIT;
		[[fallthrough]];
	case WRITE_WAIT:
		do_state(WRITE_WAIT);
		break;
	case WRITE3:
		do_state(WRITE3);
		set_idle();
		break;

	case RMW_READ0:
		do_state(READ0);
		break;
	case RMW_READ1:
		do_state(READ1);
		break;
	case RMW_READ2:
		do_state(READ2);
		state = RMW_READ_WAIT;
		[[fallthrough]];
	case RMW_READ_WAIT:
		do_state(READ_WAIT);
		break;
	case RMW_READ3:
		do_state(RMW_READ3);
		break;

	case RMW_MODIFY0:
		do_state(RMW_MODIFY0);
		break;
	case RMW_MODIFY1:
		do_state(RMW_MODIFY1);
		break;

	case RMW_WRITE0:
		do_state(WRITE0);
		break;
	case RMW_WRITE1:
		do_state(WRITE1);
		break;
	case RMW_WRITE2:
		do_state(WRITE2);
		state = RMW_WRITE_WAIT;
		[[fallthrough]];
	case RMW_WRITE_WAIT:
		do_state((WRITE_WAIT));
		break;
	case RMW_WRITE3:
		do_state(WRITE3);
		set_idle();
		break;

	default:
		throw internal_error();
	}

	advance_state();
}

void bus_manager::do_state(int state)
{
	switch (state)
	{
	case IDLE:
		if (bus.is_set(bus::BR))
		{
			// we're idle and bus is requsted - perfect time to give it up
			bus.set(bus::BG); // grant access just by setting BR flag
		}
		break;

	/* bus ready cycle */
	case READ0:
		if (check_exceptions())
			return;

		bus.func_codes(gen_func_codes());
		bus.set(bus::RW);
		bus.address(address);
		return;

	case READ1:
		bus.set(bus::AS);
		set_data_strobe_bus();
		return;

	case READ2:
		if (byte_operation)
			external_memory->init_read_byte(bus.address());
		else
			external_memory->init_read_word(bus.address());
		return;

	case READ_WAIT:
		if (external_memory->is_idle())
		{
			if (byte_operation)
				set_data_bus(external_memory->latched_byte());
			else
				set_data_bus(external_memory->latched_word());

			bus.set(bus::DTACK);
		}
		return;

	case READ3:
		clear_bus();
		return;

	/* bus read-modify-write cycles */
	case RMW_READ3:
		clear_bus();
		bus.set(bus::AS); // keep it on
		return;

	case RMW_MODIFY0:
		// IDLE CYCLE
		return;

	case RMW_MODIFY1:
		data_to_write = std::uint8_t(modify_cb(external_memory->latched_byte()));
		return;

	/* bus write cycle */
	case WRITE0:
		if (check_exceptions())
			return;
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

		if (byte_operation)
			external_memory->init_write(bus.address(), std::uint8_t(data_to_write));
		else
			external_memory->init_write(bus.address(), data_to_write);

		return;

	case WRITE_WAIT:
		if (external_memory->is_idle())
			bus.set(bus::DTACK);
		return;

	case WRITE3:
		clear_bus();
		bus.set(bus::RW);
		return;

	default:
		throw internal_error();
	}
}

void bus_manager::advance_state()
{
	switch (state)
	{
	case IDLE:
		return;

	case READ_WAIT:
	case WRITE_WAIT:
	case RMW_READ_WAIT:
	case RMW_WRITE_WAIT:
		// advance only if operation is completed (i.e. get DTACK)
		if (external_memory->is_idle())
			state = state + 1;
		return;

	default:
		state = state + 1;
	}
}

void bus_manager::set_idle()
{
	if (state == IDLE)
		return;

	state = IDLE;

	if (on_complete_cb)
		on_complete_cb();
	on_complete_cb = nullptr;

	// If callback started a new operation, throw an exception
	// chaining leads to occupying bus for 2 (or more) bus cycles,
	// we should not allow that
	assert_idle("set_idle");
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
}

std::uint8_t bus_manager::gen_func_codes() const
{
	std::uint8_t func_codes = 0;
	if (space == addr_space::DATA)
		func_codes |= 1;
	if (space == addr_space::PROGRAM)
		func_codes |= 1 << 1;
	if (regs.flags.S)
		func_codes |= 1 << 2;
	return func_codes;
}

void bus_manager::set_data_strobe_bus()
{
	if (byte_operation)
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
	if (byte_operation)
	{
		if (address_even) // uds
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
	if (false && should_rise_bus_error())
	{
		rise_bus_error();
		return true;
	}

	return false;
}

bool bus_manager::should_rise_bus_error() const
{
	// TODO: how to know max size?
	const std::uint32_t max_address = 0xFFFFFF;
	auto size = byte_operation ? 1 : 2;
	return (address & max_address) + size > max_address;
}

void bus_manager::rise_bus_error()
{
	bool read_operation = state == bus_cycle_state::READ0;
	exman.rise_bus_error({address, gen_func_codes(), read_operation, false});
	reset();
}

bool bus_manager::check_address_error()
{
	if (should_rise_address_error())
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
	bool read_operation = state == bus_cycle_state::READ0;
	bool in = space == addr_space::PROGRAM; // just to satisfy external tests
	exman.rise_address_error({address, gen_func_codes(), read_operation, in});
	reset();
}

} // namespace genesis::m68k
