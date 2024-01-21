#ifndef __IO_PORTS_CONTROLLER_H__
#define __IO_PORTS_CONTROLLER_H__

#include <memory>
#include <cstdint>

#include "input_device.h"
#include "memory/addressable.h"

namespace genesis::io_ports
{

namespace __impl
{

class data_port : public memory::addressable
{
private:
	const std::uint8_t ALL_RELEASESD = 0xFF;

	enum class state
	{
		unknown,
		first_byte,
		second_byte
	};

public:
	data_port(std::shared_ptr<input_device> dev) : m_dev(dev)
	{
	}

	/* addressable interface */
	virtual std::uint32_t max_address() const { return 0x1; }

	virtual bool is_idle() const { return true; }

	virtual void init_write(std::uint32_t /* address */, std::uint8_t data)
	{
		on_write(data);
	}

	virtual void init_write(std::uint32_t /* address */, std::uint16_t data)
	{
		on_write(data);
	}

	virtual void init_read_byte(std::uint32_t /* address */)
	{
		on_read();
	}

	virtual void init_read_word(std::uint32_t /* address */)
	{
		on_read();
	}

	virtual std::uint8_t latched_byte() const
	{
		return m_latched_data;
	}

	virtual std::uint16_t latched_word() const
	{
		// TODO: not sure what to do in this case
		return m_latched_data | (m_latched_data << 8);
	}

private:
	void on_write(std::uint16_t data)
	{
		if(data == 0x40)
			m_state = state::first_byte;
		else if(data == 0x0)
			m_state = state::second_byte;
		else
			m_state = state::unknown;
	}

	void on_read()
	{
		switch (m_state)
		{
		case state::first_byte:
			m_latched_data = first_byte();
			break;

		case state::second_byte:
			m_latched_data = second_byte();
			break;

		default:
			m_latched_data = ALL_RELEASESD;
			break;
		}
	}

	std::uint8_t first_byte()
	{
		std::uint8_t data = 0x0;
		data |= key_state(key_type::UP,    0);
		data |= key_state(key_type::DOWN,  1);
		data |= key_state(key_type::LEFT,  2);
		data |= key_state(key_type::RIGHT, 3);
		data |= key_state(key_type::B,     4);
		data |= key_state(key_type::C,     5);
		return data;
	}

	std::uint8_t second_byte()
	{
		std::uint8_t data = 0x0;
		data |= key_state(key_type::UP,    0);
		data |= key_state(key_type::DOWN,  1);
		data |= key_state(key_type::A,     4);
		data |= key_state(key_type::START, 5);
		return data;
	}

	std::uint8_t key_state(key_type key, std::uint8_t bit_position)
	{
		if(m_dev->is_key_pressed(key))
			return 0x0;
		return (1 << bit_position);
	}

private:
	std::shared_ptr<input_device> m_dev;
	state m_state = state::unknown;
	std::uint8_t m_latched_data = ALL_RELEASESD;
};

} // namespace __impl

/* * Standard 3-button controller
	* This class can be destroyed after getting data/control ports
*/
class controller
{
public:
	controller(std::shared_ptr<input_device> input_dev)
	{
		m_data_port = std::make_shared<__impl::data_port>(input_dev);
		m_control_port = std::make_shared<memory::zero_memory_unit>(0x1);
	}

	std::shared_ptr<memory::addressable> data_port()
	{
		return m_data_port;
	}

	std::shared_ptr<memory::addressable> control_port()
	{
		return m_control_port;
	}

private:
	std::shared_ptr<memory::addressable> m_data_port;
	std::shared_ptr<memory::addressable> m_control_port;
};

};

#endif // __IO_PORTS_CONTROLLER_H__
