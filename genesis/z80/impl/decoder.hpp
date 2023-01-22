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
			return decode_register(addr_mode, regs);

		case addressing_mode::immediate:
			return decode_immediate(inst, regs, mem);
		
		case addressing_mode::indirect_hl:
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			return mem.read<std::int8_t>(decode_address(addr_mode, regs, mem));
		
		default:
			throw std::runtime_error("decode_to_byte error: unsupported addresing mode " + addr_mode);
		}
	}

	static z80::memory::address decode_address(addressing_mode addr_mode, cpu_registers& regs, z80::memory& mem)
	{
		switch(addr_mode)
		{
		case addressing_mode::indirect_hl:
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
		default:
			throw std::runtime_error("decode_register error: unsupported addressing mode: " + addr_mode);
		}
	}

	static std::int8_t decode_immediate(const instruction& inst, cpu_registers& regs, z80::memory& mem)
	{
		auto addr = regs.PC;
		addr += inst.opcodes[1] == 0x0 ? 1 : 2;

		// TODO: if one of the addressing mode is indexed - we need to add 1 to offset (to skip d)

		return mem.read<std::int8_t>(addr);
	}

	// TODO: return uin16, not address!

	static memory::address decode_indirect(addressing_mode addr_mode, cpu_registers& regs)
	{
		switch (addr_mode)
		{
		case addressing_mode::indirect_hl:
			return regs.main_set.HL;
		default:
			throw std::runtime_error("decode_indirect error: unsupported addressing mode: " + addr_mode);
		}
	}

	static memory::address decode_indexed(addressing_mode addr_mode, cpu_registers& regs, z80::memory& mem)
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
		auto addressing_mode_offset = [](addressing_mode addr_mode) -> std::uint16_t
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
			case addressing_mode::indirect_hl:
			case addressing_mode::implied:
				return 0;

			case addressing_mode::immediate:
			case addressing_mode::indexed_ix:
			case addressing_mode::indexed_iy:
				return 1;
			default:
				throw std::runtime_error("advance_pc error: unsupported addressing mode: " + addr_mode);
			}
		};

		auto src_offset = addressing_mode_offset(inst.source);
		auto dest_offset = addressing_mode_offset(inst.destination);
		std::uint16_t opcode_offset = inst.opcodes[1] == 0x0 ? 1 : 2;

		regs.PC += opcode_offset + std::max(src_offset, dest_offset);
	}
};

}

#endif // __DECODER_HPP__
