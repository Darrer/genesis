#ifndef __M68K_EA_DECODER_HPP__
#define __M68K_EA_DECODER_HPP__

#include "m68k/cpu_registers.hpp"
#include "bus_manager.hpp"
#include "bus_scheduler.h"

#include <cstdint>


namespace genesis::m68k
{

class operand
{
public:
	enum class type : std::uint8_t
	{
		address_register,
		data_register,
		immediate,
		pointer,
		read_only_pointer,
	};

public:
	struct raw_pointer
	{
		raw_pointer(std::uint32_t address) : address(address) { }
		raw_pointer(std::uint32_t address, std::uint32_t value)
			: address(address), _value(value) { }

		std::uint32_t address;

		bool has_value() const
		{
			return _value.has_value();
		}

		std::uint32_t value() const
		{
			if(!has_value())
				throw internal_error();

			return _value.value();
		}

	private:
		std::optional<std::uint32_t> _value;
	};

public:
	operand(address_register& _addr_reg, size_type size) : _addr_reg(_addr_reg), _size(size) { }
	operand(data_register& _data_reg, size_type size) : _data_reg(_data_reg), _size(size) { }
	operand(std::uint32_t _imm, size_type size) : _imm(_imm), _size(size) { }
	operand(raw_pointer ptr, size_type size) : _ptr(ptr), _size(size) { }

	bool is_addr_reg() const { return _addr_reg.has_value(); }
	bool is_data_reg() const { return _data_reg.has_value(); }
	bool is_imm() const { return _imm.has_value(); }
	bool is_pointer() const { return _ptr.has_value(); }

	address_register& addr_reg()
	{
		if(!is_addr_reg()) throw internal_error();

		return _addr_reg.value().get();
	}

	data_register& data_reg()
	{
		if(!is_data_reg()) throw internal_error();

		return _data_reg.value().get();
	}

	std::uint32_t imm() const
	{
		if(!is_imm()) throw internal_error();

		return _imm.value();
	}

	raw_pointer pointer() const
	{
		if(!is_pointer()) throw internal_error();

		return _ptr.value();
	}

	size_type size() const
	{
		return _size;
	}

private:
	size_type _size;
	std::optional<std::reference_wrapper<address_register>> _addr_reg;
	std::optional<std::reference_wrapper<data_register>> _data_reg;
	std::optional<std::uint32_t> _imm;
	std::optional<raw_pointer> _ptr;
};


// effective address decoder
class ea_decoder
{
public:
	enum class flags
	{
		none,
		no_read,
	};

public:
	ea_decoder(cpu_registers& regs, bus_scheduler& scheduler)
		: regs(regs), scheduler(scheduler) { }

	bool ready() const
	{
		return res.has_value();
	}

	operand result()
	{
		if(!ready()) throw internal_error();

		return res.value();
	}

	void reset()
	{
		res.reset();
	}

	void schedule_decoding(std::uint8_t ea, size_type size, flags flags = flags::none)
	{
		if(!scheduler.is_idle())
			throw internal_error();

		res.reset();
		this->flags = flags;
		std::uint8_t mode = (ea >> 3) & 0x7;
		std::uint8_t reg = ea & 0x7;

		schedule_decoding(mode, reg, (size_type)size);
	}

private:
	void schedule_decoding(std::uint8_t mode, std::uint8_t reg, size_type size)
	{
		switch (mode)
		{
		case 0b000:
			decode_000(reg, size);
			break;

		case 0b001:
			decode_001(reg, size);
			break;

		case 0b010:
			decode_010(reg, size);
			break;

		case 0b011:
			decode_011(reg, size);
			break;

		case 0b100:
			decode_100(reg, size);
			break;

		case 0b101:
			decode_101(reg, size);
			break;

		case 0b110:
			decode_110(reg, size);
			break;

		case 0b111:
		{
		switch (reg)
		{
		case 0b000:
			decode_111_000(size);
			break;

		case 0b001:
			decode_111_001(size);
			break;

		case 0b010:
			decode_111_010(size);
			break;

		case 0b011:
			decode_111_011(size);
			break;

		case 0b100:
			decode_111_100(size);
			break;

		default: throw internal_error();
		}
			break;
		}

		default: throw internal_error();
		}
	}

private:
	/* immediately decoding */
	void decode_000(std::uint8_t reg, size_type size)
	{
		res = { regs.D(reg), size };
	}

	void decode_001(std::uint8_t reg, size_type size)
	{
		res = { regs.A(reg), size };
	}

	// Address Register Indirect Mode 
	void decode_010(std::uint8_t reg, size_type size)
	{
		schedule_read_and_save(regs.A(reg).LW, size);
	}

	// Address Register Indirect with Postincrement Mode
	void decode_011(std::uint8_t reg, size_type size)
	{
		schedule_read_and_save(regs.A(reg).LW, size);
		// TODO: if the above call rises exception, we may end up with incorrect reg
		if(flags == flags::none)
			regs.inc_addr(reg, size);
	}

	// Address Register Indirect with Predecrement Mode 
	void decode_100(std::uint8_t reg, size_type size)
	{
		if(flags == flags::none)
		{
			scheduler.wait(2);
			regs.dec_addr(reg, size);
		}

		schedule_read_and_save(regs.A(reg).LW, size);
	}

	// Address Register Indirect with Displacement Mode
	void decode_101(std::uint8_t reg, size_type size)
	{
		scheduler.prefetch_irc();

		std::uint32_t ptr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)regs.IRC);
		schedule_read_and_save(ptr, size);
	}

	// Address Register Indirect with Index (8-Bit Displacement) Mode 
	void decode_110(std::uint8_t reg, size_type size)
	{
		scheduler.wait(2);
		scheduler.prefetch_irc();

		std::uint32_t ptr = dec_brief_reg(regs.A(reg).LW, regs.IRC, regs);
		schedule_read_and_save(ptr, size);
	}

	// Absolute Short Addressing Mode 
	void decode_111_000(size_type size)
	{
		scheduler.prefetch_irc();
		schedule_read_and_save((std::int16_t)regs.IRC, size);
	}

	// Absolute Long Addressing Mode
	void decode_111_001(size_type size)
	{
		this->size = size;
		scheduler.read_imm(size_type::LONG, [this](std::uint32_t imm, size_type)
		{
			schedule_read_and_save(imm, this->size);
		});
	}

	// Program Counter Indirect with Displacement Mode
	void decode_111_010(size_type size)
	{
		scheduler.prefetch_irc();

		std::uint32_t ptr = regs.PC + (std::int16_t)regs.IRC;
		schedule_read_and_save(ptr, size);
	}

	// Program Counter Indirect with Index (8-Bit Displacement) Mode 
	void decode_111_011(size_type size)
	{
		scheduler.wait(2);
		scheduler.prefetch_irc();
		
		std::uint32_t ptr = dec_brief_reg(regs.PC, regs.IRC, regs);
		schedule_read_and_save(ptr, size);
	}

	// Immediate Data 
	void decode_111_100(size_type size)
	{
		scheduler.read_imm(size, [this](std::uint32_t imm, size_type size)
		{
			res = { imm, size };
		});
	}


private:
	/* helper methods */
	void schedule_read_and_save(std::uint32_t addr, size_type size)
	{
		if(flags == flags::no_read)
		{
			res = { operand::raw_pointer(addr), size };
		}
		else
		{
			ptr = addr;
			scheduler.read(addr, size, [this](std::uint32_t data, size_type size)
			{
				res = { operand::raw_pointer(ptr, data), size };
			});
		}
	}

private:
	struct brief_ext
	{
		brief_ext(std::uint16_t raw)
		{
			static_assert(sizeof(brief_ext) == 2);
			*reinterpret_cast<std::uint16_t*>(this) = raw;
		}

		std::uint8_t displacement;
		std::uint8_t _res : 3;
		std::uint8_t wl : 1;
		std::uint8_t reg : 3;
		std::uint8_t da : 1;
	};

public:
	static std::uint32_t dec_brief_reg(std::uint32_t base, std::uint16_t irc, cpu_registers& regs)
	{
		brief_ext ext(irc);
		base += (std::int32_t)(std::int8_t)ext.displacement;

		if(ext.wl)
			base += ext.da ? regs.A(ext.reg).LW : regs.D(ext.reg).LW;
		else
			base += (std::int16_t)(ext.da ? regs.A(ext.reg).W : regs.D(ext.reg).W);

		return base;
	}

private:
	cpu_registers& regs;
	bus_scheduler& scheduler;

	std::optional<operand> res;

	size_type size;
	std::uint32_t ptr;
	flags flags = flags::none;
};

}

#endif //__M68K_EA_DECODER_HPP__
