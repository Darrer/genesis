#ifndef __M68K_EA_DECODER_HPP__
#define __M68K_EA_DECODER_HPP__

#include <cstdint>
#include <variant>

#include "bus_manager.hpp"
#include "prefetch_queue.hpp"
#include "m68k/cpu_registers.hpp"

namespace genesis::m68k
{

class operand
{
public:
	// if operand is an address, then the actual value is read during decoding process,
	// so logically (and practically) it makes sense to store the operand's value together with the address
	class pointer
	{
	public:
		pointer(std::uint32_t addr, std::uint8_t byte) : address(addr) { _val = byte; }
		pointer(std::uint32_t addr, std::uint16_t word) : address(addr) { _val = word; }
		pointer(std::uint32_t addr, std::uint32_t long_word) : address(addr) { _val = long_word; }

		template<class T>
		T value() const
		{
			static_assert(sizeof(T) <= 4);
			static_assert(std::numeric_limits<T>::is_signed == false);

			if(!std::holds_alternative<T>(_val))
				throw std::runtime_error("operand::pointer::value error: specified type is missing");

			return std::get<T>(_val);
		}

		std::uint32_t address;
	private:
		std::variant<std::uint8_t, std::uint16_t, std::uint32_t> _val;
	};

public:
	operand(address_register& _addr_reg) : _addr_reg(_addr_reg) { }
	operand(data_register& _data_reg) : _data_reg(_data_reg) { }
	operand(pointer ptr) : _ptr(ptr) { }

	bool has_addr_reg() const { return _addr_reg.has_value(); }

	bool has_data_reg() const { return _data_reg.has_value(); }

	bool has_pointer() const { return _ptr.has_value(); }

	address_register& addr_reg()
	{
		if(!has_addr_reg())
			throw std::runtime_error("operand::addr_reg error: cannot access address register");
		
		return _addr_reg.value().get();
	}

	data_register& data_reg()
	{
		if(!has_data_reg())
			throw std::runtime_error("operand::data_reg error: cannot access data register");
		
		return _data_reg.value().get();
	}

	pointer pointer() const
	{
		if(!has_pointer())
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
		START,
		WAIT_EX1,
		DECODE_EXT1,
	};

	struct brief_ext
	{
		brief_ext(std::uint16_t raw)
		{
			static_assert(sizeof(brief_ext) == 2);
			*reinterpret_cast<std::uint16_t*>(this) = raw;
		}

		std::uint8_t displacement;
		std::uint8_t _res : 1;
		std::uint8_t scale : 2;
		std::uint8_t wl : 1;
		std::uint8_t reg : 3;
		std::uint8_t da : 1;
	};

public:
	ea_decoder(bus_manager& busm, cpu_registers& regs, prefetch_queue& pq) : busm(busm), regs(regs), pq(pq) { }

	bool ready() const
	{
		return res.has_value();
	}

	operand result()
	{
		if(!res.has_value())
			throw std::runtime_error("ea_decoder::result error: result is not available");

		return res.value();
	}

	void reset()
	{
		// TODO: what if we started read bus cycle?
		state = IDLE;
		res.reset();
		reg = mode = size = 0;
		ext1 = 0;
	}

	void decode(std::uint8_t ea, std::uint8_t size = 1)
	{
		if(state != IDLE)
			throw std::runtime_error("ea_decoder::decode error: cannot start new decoding till the previous request is finished");
		
		if(!busm.is_idle())
			throw std::runtime_error("ea_decoder::decode error: cannot start decoding as bus_manager is busy");

		reset();

		reg = ea & 0x7;
		mode = (ea >> 3) & 0x7;
		this->size = size;

		// check if can decode immediately
		switch (mode)
		{
		case 0b000:
			decode_000();
			break;

		case 0b001:
			decode_001();
			break;

		default:
			state = START;
		}
	}

	void cycle()
	{
		switch (state)
		{
		case IDLE:
			break;

		case START:
			// TODO: need access to fetch queue here
			busm.init_read_word(regs.PC);
			state = WAIT_EX1; break;
		
		case WAIT_EX1:
			if(!busm.is_idle()) break;

			ext1 = busm.letched_word();
			regs.PC += sizeof(ext1);
			state = DECODE_EXT1;
			[[fallthrough]];
		case DECODE_EXT1:
		{
			switch (mode)
			{
			case 0b101:
			{
				decode_101();
				state = IDLE; break;
			}

			case 0b110:
			{
				decode_110();
				state = IDLE; break;
			}

			default:
				state = IDLE;
				throw std::runtime_error("internal error: unknown ea mode " + std::to_string(mode));
			}

			break;
		}

		default:
			throw std::runtime_error("ea_docoder::cycle internal error: unknown state");
		}
	}

private:
	/* immediately decodeing */
	void decode_000()
	{
		res = { regs.D(reg) };
	}

	void decode_001()
	{
		res = { regs.A(reg) };
	}

	/* neec 1 extension word to decode */
	void decode_101()
	{
		std::int32_t addr = regs.A(reg).LW;
		addr += (std::int16_t)ext1;

		res = { regs.D0 };
	}

	void decode_110()
	{
		std::int32_t addr = regs.A(reg).LW;

		brief_ext ext(ext1);
		addr += (std::int32_t)(std::int8_t)ext.displacement;
		addr += (std::int32_t)dec_brief_reg(ext);

		res = { regs.D0 };
	}

private:
	std::int32_t dec_brief_reg(brief_ext ext)
	{
		// std::cout << "Scale value: " << (int)ext.scale << std::endl;
		std::int32_t scale = 1 ; // 1 << ext.scale;

		if(ext.wl)
		{
			std::int32_t res = ext.da ? regs.A(ext.reg).LW : regs.D(ext.reg).LW;
			return res * scale;
		}
		else
		{
			std::int16_t w = ext.da ? regs.A(ext.reg).W : regs.D(ext.reg).W;
			std::int32_t res = w;

			return res * scale;
		}
	}

private:
	bus_manager& busm;
	cpu_registers& regs;
	prefetch_queue& pq;
	std::optional<operand> res;

	decode_state state = IDLE;
	std::uint8_t mode = 0;
	std::uint8_t reg = 0;
	std::uint8_t size = 1;

	std::uint16_t ext1 = 0;
};

}

#endif //__M68K_EA_DECODER_HPP__
