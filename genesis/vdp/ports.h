#ifndef __VDP_PORTS_H__
#define __VDP_PORTS_H__

#include <optional>

#include "settings.h"
#include "register_set.h"

#include "exception.hpp"


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


class ports
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
	ports(register_set& regs) : regs(regs) { }

	bool is_idle() const;

	void init_read_control();
	void init_write_control(std::uint8_t);
	void init_write_control(std::uint16_t);

	void init_read_data();
	void init_write_data(std::uint8_t);
	void init_write_data(std::uint16_t);

	std::uint16_t read_result();

	// interface for vdp
	std::optional<control_write_request>& pending_control_write_requet() { return _control_write_request; }
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

}

#endif // __VDP_PORTS_H__