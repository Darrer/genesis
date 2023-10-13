#ifndef __M68K_TEST_INT_DEV_H__
#define __M68K_TEST_INT_DEV_H__

#include "m68k/interrupting_device.h"


namespace genesis::test
{

class int_dev : public genesis::m68k::interrupting_device
{
public:
	int_dev()
	{
		set_uninitialized();
	}

	virtual bool is_idle() const override
	{
		return true;
	}

	virtual void init_interrupt_ack(std::uint8_t priority) override
	{
		this->priority = priority;
	}

	virtual m68k::interrupt_type interrupt_type() override
	{
		return type;
	}

	virtual std::uint8_t vector_number() override
	{
		switch (type)
		{
		case m68k::interrupt_type::spurious:
			return m68k::spurious_vector_number;
		case m68k::interrupt_type::autovectored:
			return autovectored(priority);
		case m68k::interrupt_type::vectored:
			return vec;
		default: throw internal_error();
		}
	}

	/* test methods */
	void set_spurious()
	{
		type = m68k::interrupt_type::spurious;
	}

	void set_vectored(std::uint8_t vector)
	{
		type = m68k::interrupt_type::vectored;
		vec = vector;
	}

	void set_autovectored()
	{
		type = m68k::interrupt_type::autovectored;
	}

	void set_uninitialized()
	{
		type = m68k::interrupt_type::vectored;
		vec = m68k::uninitialized_vector_number;
	}

private:
	m68k::interrupt_type type;
	std::uint8_t priority;
	std::uint8_t vec;
};

}

#endif // __M68K_TEST_INT_DEV_H__