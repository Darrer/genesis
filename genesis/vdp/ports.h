#ifndef __VDP_PORTS_H__
#define __VDP_PORTS_H__

#include "memory/addressable.h"
#include "register_set.h"
#include "settings.h"

#include "exception.hpp"
#include "string_utils.hpp"

#include <optional>


namespace genesis::vdp
{


struct control_write_request
{
	/* 	enum class target
		{
			register,
			address,
			address_second
		}; */

	std::uint16_t data;
	bool first_word;
};


class ports : public memory::addressable
{
private:
	enum class request
	{
		none,

		read_control,
		write_control,

		read_data,
		write_data,
	};

public:
	ports(register_set& regs) : regs(regs)
	{
	}

	/* addressable interface */
	std::uint32_t max_address() const override
	{
		return 0x1F;
	}

	void init_write(std::uint32_t address, std::uint8_t data) override
	{
		// TODO: should we support 1 byte access?
		if(address < 4)
			init_write_data(data);
		else if(address < 8)
			init_write_control(data);
		else
			throw not_implemented("Write is not supported yet: " + su::hex_str(address));
	}

	void init_write(std::uint32_t address, std::uint16_t data) override
	{
		if(address < 4)
			init_write_data(data);
		else if(address < 8)
			init_write_control(data);
		else
			throw not_implemented("Write is not supported yet: " + su::hex_str(address));
	}

	void init_read_byte(std::uint32_t address) override
	{
		// TODO: should we support 1 byte access?
		if(address < 4)
			init_read_data();
		else if(address < 8)
			init_read_control();
		else
			throw not_implemented("Read is not supported yet: " + su::hex_str(address));
	}

	void init_read_word(std::uint32_t address) override
	{
		if(address < 4)
			init_read_data();
		else if(address < 8)
			init_read_control();
		else
			throw not_implemented("Read is not supported yet: " + su::hex_str(address));
	}

	std::uint8_t latched_byte() const override
	{
		// TODO:
		return read_result() & 0xFF;
	}

	std::uint16_t latched_word() const override
	{
		return read_result();
	}

	/* VDP Ports interface */

	bool is_idle() const override;

	void init_read_control();
	void init_write_control(std::uint8_t); // TODO: remove
	void init_write_control(std::uint16_t);

	void init_read_data();
	void init_write_data(std::uint8_t); // TODO: remove
	void init_write_data(std::uint16_t);

	std::uint16_t read_result() const;

	// interface for vdp
	std::optional<control_write_request>& pending_control_write_requet()
	{
		return _control_write_request;
	}

	void cycle();
	void reset();


	std::uint16_t read_control()
	{
		control_pending = false;
		return regs.sr_raw;
	}

private:
	void start(request _req);

private:
	register_set& regs;
	request req = request::none;

	std::uint16_t data_to_write = 0;
	bool reading_control_port = false;
	std::optional<std::uint16_t> read_data;

	// true if we're waiting for the 2nd control word
	bool control_pending = false;

	std::optional<control_write_request> _control_write_request;
};

} // namespace genesis::vdp

#endif // __VDP_PORTS_H__