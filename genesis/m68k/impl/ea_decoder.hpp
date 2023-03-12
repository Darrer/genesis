#ifndef __M68K_EA_DECODER_HPP__
#define __M68K_EA_DECODER_HPP__

#include <cstdint>
#include <variant>
#include <iostream>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "m68k/cpu_registers.hpp"

namespace genesis::m68k
{

class operand
{
public:
	struct pointer
	{
		// pointer(std::uint32_t addr, std::uint8_t value) : address(addr), value(value) { }
		// pointer(std::uint32_t addr, std::uint16_t value) : address(addr), value(value) { }
		// pointer(std::uint32_t addr, std::uint32_t value) : address(addr), value(value) { }

		// TODO: In some cases operand has a read-only address
		std::uint32_t address;
		std::uint32_t value;
	};

public:
	operand(address_register& _addr_reg) : _addr_reg(_addr_reg) { }
	operand(data_register& _data_reg) : _data_reg(_data_reg) { }
	operand(pointer ptr) : _ptr(ptr) { }

	bool is_addr_reg() const { return _addr_reg.has_value(); }
	bool is_data_reg() const { return _data_reg.has_value(); }
	bool is_pointer() const { return _ptr.has_value(); }

	address_register& addr_reg()
	{
		if(!is_addr_reg())
			throw std::runtime_error("operand::addr_reg error: cannot access address register");
		
		return _addr_reg.value().get();
	}

	data_register& data_reg()
	{
		if(!is_data_reg())
			throw std::runtime_error("operand::data_reg error: cannot access data register");
		
		return _data_reg.value().get();
	}

	pointer pointer() const
	{
		if(!is_pointer())
			throw std::runtime_error("operand::pointer error: cannot access pointer");
		
		return _ptr.value();
	}

private:
	std::optional<std::reference_wrapper<address_register>> _addr_reg;
	std::optional<std::reference_wrapper<data_register>> _data_reg;
	std::optional<class operand::pointer> _ptr;
};


// effective address decoder
class ea_decoder
{
private:
	enum decode_state
	{
		IDLE,
		DECODE0,
		PREFETCH_IRC,
		PREFETCH_IRC_AND_IDLE,
		READ_PTR,
	};

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
	ea_decoder(bus_manager& busm, cpu_registers& regs, prefetch_queue& pq) : busm(busm), regs(regs), pq(pq) { }

	bool is_idle() const
	{
		if(state == IDLE)
			return true;

		if(state == PREFETCH_IRC_AND_IDLE && pq.is_idle())
			return true;
		
		return false;
	}

	bool ready() const
	{
		// TODO: should we return result once it's available?
		// Or should we wait till prefetch is over?
		return res.has_value() && is_idle();
	}

	operand result()
	{
		if(!ready())
			throw std::runtime_error("ea_decoder::result error: result is not available");

		return res.value();
	}

	void reset()
	{
		// TODO: what if we started read bus cycle?
		state = IDLE;
		res.reset();
		reg = mode = size = 0;
		dec_stage = 0;
		ext1 = 0;
	}

	void decode(std::uint8_t ea, std::uint8_t size = 1)
	{
		if(!is_idle())
			throw std::runtime_error("ea_decoder::decode error: cannot start new decoding till the previous request is finished");

		if(!busm.is_idle())
			throw std::runtime_error("ea_decoder::decode error: cannot start decoding as bus_manager is busy");

		reset();

		reg = ea & 0x7;
		mode = (ea >> 3) & 0x7;
		this->size = size;

		// std::cout << "Decoding: " << su::bin_str(mode) << std::endl;

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
		
		case 0b101:
			decode_101();
			break;

		default:
			state = DECODE0;
		}
	}

	void cycle()
	{
		switch (state)
		{
		case IDLE:
			break;

		case READ_PTR:
			if(!busm.is_idle()) break;
			save_pointer_and_idle();
			break;

		case PREFETCH_IRC_AND_IDLE:
			if(pq.is_idle())
				state = IDLE; // TODO: lose 1 cycle here
			break;

		case PREFETCH_IRC:
			if(!pq.is_idle()) break;
			state = DECODE0;
			[[fallthrough]];

		// need 1 extension word
		case DECODE0:
		{
			switch (mode)
			{
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
			
			case 0b100:
				decode_111_100();
				break;
			
			case 0b001:
				decode_111_001();
				break;

			case 0b011:
				decode_111_011();
				break;

			case 0b010:
				decode_111_010();
				break;

			default:
				state = IDLE;
				throw std::runtime_error("internal error: unknown ea mode 111 " + std::to_string(reg));
			}

				break;
			}

			default:
				state = IDLE;
				throw std::runtime_error("internal error: unknown ea mode " + std::to_string(mode));
			}

			break;
		}

		default:
			state = IDLE;
			throw std::runtime_error("ea_docoder::cycle internal error: unknown state");
		}
	}

private:
	void prefetch_irc()
	{
		pq.init_fetch_irc();
		regs.PC += 2;
		state = PREFETCH_IRC;
	}

	void prefetch_and_idle()
	{
		pq.init_fetch_irc();
		regs.PC += 2;
		state = PREFETCH_IRC_AND_IDLE;
	}

	void read_pointer_and_idle(std::uint32_t addr)
	{
		if(size == 1)
			busm.init_read_byte(addr, [&]() { save_pointer_and_idle(); } );
		else if(size == 2)
			busm.init_read_word(addr, [&]() { save_pointer_and_idle(); });
		else
			throw std::runtime_error("read long word is not implemented yet");

		ptr = addr;
		state = READ_PTR;
	}

	void save_pointer_and_idle()
	{
		if(size == 1)
			res = { {ptr, busm.letched_byte() } };
		else if(size == 2)
			res = { {ptr, busm.letched_word() } };
		else
			throw std::runtime_error("read long word is not implemented yet");

		state = IDLE;
	}

private:
	/* immediately decoding */
	void decode_000()
	{
		res = { regs.D(reg) };
	}

	void decode_001()
	{
		res = { regs.A(reg) };
	}

	/* need 1 bus read cycle */

	// Address Register Indirect Mode 
	void decode_010()
	{
		read_pointer_and_idle(regs.A(reg).LW);
	}

	// Address Register Indirect with Postincrement Mode
	void decode_011()
	{
		read_pointer_and_idle(regs.A(reg).LW);
		regs.A(reg).LW += size;
	}

	// Address Register Indirect with Predecrement Mode 
	void decode_100()
	{
		switch (dec_stage++)
		{
		case 0: break;
		case 1: break; // 2 IDLE cycles
		case 2:
			// TODO: check
			if(reg == 0b111 && size == 1)
				regs.A(reg).LW -= 2; // to maintain alignment
			else
				regs.A(reg).LW -= size;
			read_pointer_and_idle(regs.A(reg).LW);
			break;
		default: throw std::runtime_error("ea_decoder::decode_100 internal error: unknown stage");
		}
	}

	// Address Register Indirect with Displacement Mode
	void decode_101()
	{
		switch (dec_stage++)
		{
		case 0:
			ptr = (std::int32_t)regs.A(reg).LW + std::int32_t((std::int16_t)pq.IRC);
			prefetch_irc();
			break;
		case 1:
			read_pointer_and_idle(ptr);
			break;
		default: throw std::runtime_error("ea_decoder::decode_101 internal error: unknown stage");
		}
	}

	// Address Register Indirect with Index (8-Bit Displacement) Mode 
	void decode_110()
	{
		switch (dec_stage++)
		{
		case 0: break;
		case 1: break; // 2 IDLE cycles
		case 2:
		{
			brief_ext ext(pq.IRC);
			ptr = regs.A(reg).LW;
			ptr += (std::int32_t)(std::int8_t)ext.displacement;
			ptr += dec_brief_reg(ext);
			prefetch_irc();
			break;
		}
		case 3:
			read_pointer_and_idle(ptr);
			break;

		default: throw std::runtime_error("ea_decoder::decode_110 internal error: unknown stage");
		}
	}

	// Absolute Short Addressing Mode 
	void decode_111_000()
	{
		switch (dec_stage++)
		{
		case 0:
			ptr = (std::int16_t)pq.IRC;
			prefetch_irc();
			break;
		case 1:
			read_pointer_and_idle(ptr);
			break;
		default: throw std::runtime_error("ea_decoder::decode_111_000 internal error: unknown stage");
		}
	}

	// Immediate Data 
	void decode_111_100()
	{
		if(size != 1 && size != 2)
			throw std::runtime_error("ea_decoder::decode_111_100 not implemented yet");

		switch (dec_stage++)
		{
		case 0:
			if(size == 1)
				res = { {regs.PC, (std::uint8_t)(pq.IRC & 0xFF) } }; // TODO
			else
				res = { {regs.PC, pq.IRC } }; // TODO
			prefetch_and_idle();
			break;

		default: throw std::runtime_error("ea_decoder::decode_111_100 internal error: unknown stage");
		}
	}

	// Program Counter Indirect with Displacement Mode
	void decode_111_010()
	{
		switch (dec_stage++)
		{
		case 0:
			ptr = regs.PC + (std::int16_t)pq.IRC;
			prefetch_irc();
			break;
		case 1:
			read_pointer_and_idle(ptr);
			break;
		default: throw std::runtime_error("ea_decoder::decode_111_010 internal error: unknown stage");
		}
	}

	// Program Counter Indirect with Index (8-Bit Displacement) Mode 
	void decode_111_011()
	{
		switch (dec_stage++)
		{
		case 0: break;
		case 1: break; // 2 IDLE cycles
		case 2:
		{
			brief_ext ext(pq.IRC);
			ptr = regs.PC;
			ptr += (std::int32_t)(std::int8_t)ext.displacement;
			ptr += dec_brief_reg(ext);
			prefetch_irc();
			break;
		}
		case 3:
			read_pointer_and_idle(ptr);
			break;

		default: throw std::runtime_error("ea_decoder::decode_110 internal error: unknown stage");
		}
	}

	// Absolute Long Addressing Mode
	void decode_111_001()
	{
		switch (dec_stage++)
		{
		case 0:
			ptr = (std::uint16_t)pq.IRC;
			prefetch_irc();
			break;
		case 1:
			ptr = (ptr << 16) | (std::uint16_t)pq.IRC;
			prefetch_irc();
			break;
		case 2:
			read_pointer_and_idle(ptr);
			break;
		default: throw std::runtime_error("ea_decoder::decode_111_010 internal error: unknown stage");
		}
	}

private:
	std::int32_t dec_brief_reg(brief_ext ext)
	{
		if(ext.wl)
			return ext.da ? regs.A(ext.reg).LW : regs.D(ext.reg).LW;
		return (std::int16_t)(ext.da ? regs.A(ext.reg).W : regs.D(ext.reg).W);
	}

private:
	bus_manager& busm;
	cpu_registers& regs;
	prefetch_queue& pq;
	std::optional<operand> res;

	decode_state state = IDLE;
	std::uint8_t dec_stage = 0;
	std::uint8_t mode = 0;
	std::uint8_t reg = 0;
	std::uint8_t size = 1;

	std::uint32_t ptr = 0;
	std::uint16_t ext1 = 0;
};

}

#endif //__M68K_EA_DECODER_HPP__
