#ifndef __M68K_EA_DECODER_HPP__
#define __M68K_EA_DECODER_HPP__

#include <cstdint>
#include <iostream>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "m68k/cpu_registers.hpp"

#include "base_unit.h"

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
	operand(address_register& _addr_reg, std::uint8_t size) : _addr_reg(_addr_reg), _size(size) { }
	operand(data_register& _data_reg, std::uint8_t size) : _data_reg(_data_reg), _size(size) { }
	operand(std::uint32_t _imm, std::uint8_t size) : _imm(_imm), _size(size) { }
	operand(raw_pointer ptr, std::uint8_t size) : _ptr(ptr), _size(size) { }

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

	std::uint8_t size() const
	{
		return _size;
	}

private:
	std::uint8_t _size;
	std::optional<std::reference_wrapper<address_register>> _addr_reg;
	std::optional<std::reference_wrapper<data_register>> _data_reg;
	std::optional<std::uint32_t> _imm;
	std::optional<raw_pointer> _ptr;
};


// effective address decoder
class ea_decoder : public base_unit
{
public:
	ea_decoder(bus_manager& busm, cpu_registers& regs, prefetch_queue& pq, m68k::bus_scheduler& scheduler)
		: base_unit(regs, busm, pq, scheduler) { }

	bool ready() const
	{
		return res.has_value() && is_idle();
	}

	// TODO: override reset

	operand result()
	{
		if(!ready()) throw internal_error();

		return res.value();
	}

	void decode(std::uint8_t ea, std::uint8_t size)
	{
		if(!is_idle() || !busm.is_idle())
			throw internal_error();

		res.reset();
		reg = ea & 0x7;
		mode = (ea >> 3) & 0x7;
		this->size = size;

		decoding();
	}

protected:
	void on_cycle() override
	{
		// we're not expected to be called
		throw internal_error();
	}

private:
	void decoding()
	{
		switch (mode)
		{
		case 0b000:
			decode_000();
			break;

		case 0b001:
			decode_001();
			break;

		case 0b010:
			decode_010();
			break;

		case 0b011:
			decode_011();
			break;

		case 0b100:
			decode_100();
			break;

		case 0b101:
			decode_101();
			break;

		case 0b110:
			decode_110();
			break;

		case 0b111:
		{
		switch (reg)
		{
		case 0b000:
			decode_111_000();
			break;

		case 0b001:
			decode_111_001();
			break;

		case 0b010:
			decode_111_010();
			break;

		case 0b011:
			decode_111_011();
			break;

		case 0b100:
			decode_111_100();
			break;

		default:
			throw not_implemented("111 " + std::to_string(reg));
		}
			break;
		}

		default: throw internal_error();
		}
	}

private:
	/* immediately decoding */
	void decode_000()
	{
		res = { regs.D(reg), size };
	}

	void decode_001()
	{
		res = { regs.A(reg), size };
	}

	// Address Register Indirect Mode 
	void decode_010()
	{
		read_pointer_and_idle(regs.A(reg).LW);
	}

	// Address Register Indirect with Postincrement Mode
	void decode_011()
	{
		read_pointer_and_idle(regs.A(reg).LW);
		// TODO: if the above call rises exception, we may end up with incorrect reg
		inc_addr(reg, size);
	}

	// Address Register Indirect with Predecrement Mode 
	void decode_100()
	{
		wait(2);
		dec_addr(reg, size);
		read_pointer_and_idle(regs.A(reg).LW);
	}

	// Address Register Indirect with Displacement Mode
	void decode_101()
	{
		ptr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)pq.IRC);
		prefetch_irc();
		read_pointer_and_idle(ptr);
	}

	// Address Register Indirect with Index (8-Bit Displacement) Mode 
	void decode_110()
	{
		wait(2);
		prefetch_irc();
		ptr = dec_brief_reg(regs.A(reg).LW);
		read_pointer_and_idle(ptr);
	}

	// Absolute Short Addressing Mode 
	void decode_111_000()
	{
		prefetch_irc();
		read_pointer_and_idle((std::int16_t)pq.IRC);
	}

	// Absolute Long Addressing Mode
	void decode_111_001()
	{
		read_imm(4 /* long word */, [this]()
		{
			read_pointer_and_idle(imm);
		} );
	}

	// Program Counter Indirect with Displacement Mode
	void decode_111_010()
	{
		ptr = regs.PC + (std::int16_t)pq.IRC;
		prefetch_irc();
		read_pointer_and_idle(ptr);
	}

	// Program Counter Indirect with Index (8-Bit Displacement) Mode 
	void decode_111_011()
	{
		wait(2);
		prefetch_irc();
		ptr = dec_brief_reg(regs.PC);
		read_pointer_and_idle(ptr);
	}

	// Immediate Data 
	void decode_111_100()
	{
		read_imm_and_idle(size, [&]() { res = { imm, size }; });	
	}


private:
	/* helper methods */
	void read_pointer_and_idle(std::uint32_t addr)
	{
		ptr = addr;
		auto cb = [this]() { res = { operand::raw_pointer(ptr, data), size }; };
		read_and_idle(addr, size, cb);
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

	std::uint32_t dec_brief_reg(std::uint32_t base) const
	{
		brief_ext ext(pq.IRC);
		base += (std::int32_t)(std::int8_t)ext.displacement;

		if(ext.wl)
			base += ext.da ? regs.A(ext.reg).LW : regs.D(ext.reg).LW;
		else
			base += (std::int16_t)(ext.da ? regs.A(ext.reg).W : regs.D(ext.reg).W);

		return base;
	}

private:
	std::optional<operand> res;

	std::uint8_t mode;
	std::uint8_t reg;
	std::uint8_t size;

	std::uint32_t ptr;
};

}

#endif //__M68K_EA_DECODER_HPP__
