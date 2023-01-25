#ifndef __DECODER_HPP__
#define __DECODER_HPP__

#include <cstdint>
#include "../cpu_registers.hpp"
#include "../cpu.h"
#include "instructions.hpp"


namespace genesis::z80
{

class decoder
{
public:
	static std::int8_t decode_to_byte(addressing_mode addr_mode, const instruction& inst, cpu_registers& regs, z80::memory& mem)
	{
		switch(addr_mode)
		{
		case addressing_mode::register_a:
		case addressing_mode::register_b:
		case addressing_mode::register_c:
		case addressing_mode::register_d:
		case addressing_mode::register_e:
		case addressing_mode::register_h:
		case addressing_mode::register_l:
		case addressing_mode::register_i:
		case addressing_mode::register_r:
			return decode_register(addr_mode, regs);

		case addressing_mode::immediate:
			return decode_immediate(inst, regs, mem);

		case addressing_mode::immediate_ext:
		case addressing_mode::indirect_hl:
		case addressing_mode::indirect_bc:
		case addressing_mode::indirect_de:
		case addressing_mode::indirect_sp:
		case addressing_mode::indirect_af:
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			return mem.read<std::int8_t>(decode_address(addr_mode, inst, regs, mem));

		default:
			throw std::runtime_error("decode_to_byte error: unsupported addresing mode " + addr_mode);
		}
	}

	static std::int16_t decode_two_bytes(addressing_mode addr_mode, const instruction& inst, cpu_registers& regs, z80::memory& mem)
	{
		switch(addr_mode)
		{
		case addressing_mode::immediate_ext:
			return decode_immediate<std::int16_t>(inst, regs, mem);

		case addressing_mode::indirect_hl:
		case addressing_mode::indirect_bc:
		case addressing_mode::indirect_de:
		case addressing_mode::indirect_sp:
		case addressing_mode::indirect_af:
			return decode_indirect(addr_mode, regs);

		default:
			throw std::runtime_error("decode_two_bytes error: unsupported addresing mode " + addr_mode);
		}
	}

	static z80::memory::address decode_address(addressing_mode addr_mode, const instruction& inst, cpu_registers& regs, z80::memory& mem)
	{
		switch(addr_mode)
		{
		case addressing_mode::immediate_ext:
			return decode_immediate<z80::memory::address>(inst, regs, mem);

		case addressing_mode::indirect_hl:
		case addressing_mode::indirect_bc:
		case addressing_mode::indirect_de:
		case addressing_mode::indirect_sp:
		case addressing_mode::indirect_af:
			return decode_indirect(addr_mode, regs);

		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			return decode_indexed(addr_mode, regs, mem);
		
		default:
			throw std::runtime_error("decode_to_byte error: unsupported addresing mode " + addr_mode);
		}
	}

	/* required minimum */

	static std::int8_t& decode_register(addressing_mode addr_mode, cpu_registers& regs)
	{
		switch (addr_mode)
		{
		case addressing_mode::register_a:
			return regs.main_set.A;
		case addressing_mode::register_b:
			return regs.main_set.B;
		case addressing_mode::register_c:
			return regs.main_set.C;
		case addressing_mode::register_d:
			return regs.main_set.D;
		case addressing_mode::register_e:
			return regs.main_set.E;
		case addressing_mode::register_h:
			return regs.main_set.H;
		case addressing_mode::register_l:
			return regs.main_set.L;
		case addressing_mode::register_i:
			return regs.I;
		case addressing_mode::register_r:
			return regs.R;
		default:
			throw std::runtime_error("decode_register error: unsupported addressing mode: " + addr_mode);
		}
	}

	template<class T = std::int8_t>
	static T decode_immediate(const instruction& inst, cpu_registers& regs, z80::memory& mem)
	{
		static_assert(sizeof(T) <= 2);

		auto addr = regs.PC;
		addr += inst.opcodes[1] == 0x0 ? 1 : 2;

		// check if addressing mode indexed combined with addressing mode immediate
		if(is_indexed(inst.source) || is_indexed(inst.destination))
		{
			// in this case after opcode we have displacement for indexed
			// and only then immediate operand
			addr += 1; // to skip displacement
		}

		return mem.read<T>(addr);
	}

	static std::int16_t& decode_indirect(addressing_mode addr_mode, cpu_registers& regs)
	{
		switch (addr_mode)
		{
		case addressing_mode::indirect_hl:
			return regs.main_set.HL;
		case addressing_mode::indirect_bc:
			return regs.main_set.BC;
		case addressing_mode::indirect_de:
			return regs.main_set.DE;
		case addressing_mode::indirect_sp:
			return (std::int16_t&)regs.SP;
		case addressing_mode::indirect_af:
			return regs.main_set.AF;
		default:
			throw std::runtime_error("decode_indirect error: unsupported addressing mode: " + addr_mode);
		}
	}

	static std::uint16_t decode_indexed(addressing_mode addr_mode, cpu_registers& regs, z80::memory& mem)
	{
		switch (addr_mode)
		{
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
		{
			auto d = mem.read<std::int8_t>(regs.PC + 2);
			auto base = addr_mode == addressing_mode::indexed_ix ? regs.IX : regs.IY;

			return base + d;
		}
		default:
			throw std::runtime_error("decode_indexed error: unsupported addressing mode: " + addr_mode);
		}
	}

	static void advance_pc(const instruction& inst, cpu_registers& regs)
	{
		auto addressing_mode_size = [](addressing_mode addr_mode) -> std::uint16_t
		{
			switch (addr_mode)
			{
			case addressing_mode::register_a:
			case addressing_mode::register_b:
			case addressing_mode::register_c:
			case addressing_mode::register_d:
			case addressing_mode::register_e:
			case addressing_mode::register_h:
			case addressing_mode::register_l:
			case addressing_mode::register_i:
			case addressing_mode::register_r:
			case addressing_mode::indirect_hl:
			case addressing_mode::indirect_bc:
			case addressing_mode::indirect_de:
			case addressing_mode::indirect_sp:
			case addressing_mode::indirect_af:
			case addressing_mode::implied:
				return 0;

			case addressing_mode::immediate:
			case addressing_mode::indexed_ix:
			case addressing_mode::indexed_iy:
				return 1;

			case addressing_mode::immediate_ext:
				return 2;
			default:
				throw std::runtime_error("advance_pc error: unsupported addressing mode: " + addr_mode);
			}
		};

		std::uint16_t inst_size = inst.opcodes[1] == 0x0 ? 1 : 2;
		regs.PC += inst_size;

		auto src_size = addressing_mode_size(inst.source);
		auto dest_size = addressing_mode_size(inst.destination);

		// we have to add operands' size if addressing mode is indexed and immediate 
		if((is_indexed(inst.source) && is_immediate(inst.destination)) ||
			(is_indexed(inst.destination) && is_immediate(inst.source)))
		{
			// in this case after opcode we have displacement (1 byte) and n/nn (1 or 2 bytes)
			regs.PC += src_size + dest_size;
		}
		else
		{
			regs.PC += std::max(src_size, dest_size);
		}
	}

	/* helper methods */
private:
	static bool is_indexed(addressing_mode addr_mode)
	{
		return addr_mode == addressing_mode::indexed_ix || addr_mode == addressing_mode::indexed_iy;
	}

	static bool is_immediate(addressing_mode addr_mode)
	{
		return addr_mode == addressing_mode::immediate || addr_mode == addressing_mode::immediate_ext;
	}
};

}

#endif // __DECODER_HPP__
