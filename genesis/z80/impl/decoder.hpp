#ifndef __DECODER_HPP__
#define __DECODER_HPP__

#include <cstdint>

#include "z80/cpu_registers.hpp"
#include "z80/cpu.h"
#include "instructions.hpp"


namespace genesis::z80
{

#define unsupported_addresing_mode(addr_mode) \
	throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + " error: unsupported addressing mode " + std::to_string(addr_mode))

class decoder
{
public:
	decoder(z80::cpu& cpu) : mem(cpu.memory()), regs(cpu.registers())
	{
	}

	std::int8_t decode_byte(addressing_mode addr_mode, instruction inst)
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
		case addressing_mode::register_ixh:
		case addressing_mode::register_ixl:
		case addressing_mode::register_iyh:
		case addressing_mode::register_iyl:
			return decode_reg_8(addr_mode);

		case addressing_mode::immediate:
			return decode_immediate(inst);

		case addressing_mode::immediate_ext:
		case addressing_mode::indirect_hl:
		case addressing_mode::indirect_bc:
		case addressing_mode::indirect_de:
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			return mem.read<std::int8_t>(decode_address(addr_mode, inst));

		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	std::int16_t decode_2_bytes(addressing_mode addr_mode, instruction inst)
	{
		switch(addr_mode)
		{
		case addressing_mode::immediate_ext:
			return decode_immediate_ext(inst);

		case addressing_mode::register_af:
		case addressing_mode::register_bc:
		case addressing_mode::register_de:
		case addressing_mode::register_hl:
		case addressing_mode::register_sp:
		case addressing_mode::register_ix:
		case addressing_mode::register_iy:
			return decode_reg_16(addr_mode);

		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	z80::memory::address decode_address(addressing_mode addr_mode, instruction inst)
	{
		switch(addr_mode)
		{
		case addressing_mode::immediate_ext:
			return decode_immediate_ext(inst);

		case addressing_mode::indirect_hl:
		case addressing_mode::indirect_bc:
		case addressing_mode::indirect_de:
			return decode_indirect(addr_mode);

		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
			return decode_indexed(addr_mode);

		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	/* required minimum */

	std::int8_t& decode_reg_8(addressing_mode addr_mode)
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
		case addressing_mode::register_ixh:
			return regs.IXH;
		case addressing_mode::register_ixl:
			return regs.IXL;
		case addressing_mode::register_iyh:
			return regs.IYH;
		case addressing_mode::register_iyl:
			return regs.IYL;
		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	std::int16_t& decode_reg_16(addressing_mode addr_mode)
	{
		switch (addr_mode)
		{
		case addressing_mode::register_af:
			return regs.main_set.AF;
		case addressing_mode::register_bc:
			return regs.main_set.BC;
		case addressing_mode::register_de:
			return regs.main_set.DE;
		case addressing_mode::register_hl:
			return regs.main_set.HL;
		case addressing_mode::register_sp:
			return regs.SP;
		case addressing_mode::register_ix:
			return regs.IX;
		case addressing_mode::register_iy:
			return regs.IY;
		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	template<class T = std::int8_t>
	T decode_immediate(instruction inst)
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

	std::int16_t decode_immediate_ext(instruction inst)
	{
		return decode_immediate<std::int16_t>(inst);
	}

	z80::memory::address decode_indirect(addressing_mode addr_mode)
	{
		switch (addr_mode)
		{
		case addressing_mode::indirect_bc:
			return regs.main_set.BC;
		case addressing_mode::indirect_de:
			return regs.main_set.DE;
		case addressing_mode::indirect_hl:
			return regs.main_set.HL;
		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	z80::memory::address decode_indexed(addressing_mode addr_mode)
	{
		switch (addr_mode)
		{
		case addressing_mode::indexed_ix:
		case addressing_mode::indexed_iy:
		{
			auto d = mem.read<std::int8_t>(regs.PC + 2);
			z80::memory::address base = addr_mode == addressing_mode::indexed_ix ? regs.IX : regs.IY;

			return base + d;
		}
		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	std::uint8_t decode_cc(instruction inst)
	{
		// NOTE: always assume constant cc offset for all instructions
		return (inst.opcodes[0] & 0b00111000) >> 3;
	}

	std::uint8_t decode_bit(addressing_mode addr_mode, instruction inst)
	{
		switch (addr_mode)
		{
		case addressing_mode::bit:
			return (inst.opcodes[1] & 0b00111000) >> 3;
		case addressing_mode::immediate_bit:
			return (decode_immediate<std::uint8_t>(inst) & 0b00111000) >> 3;
		default:
			unsupported_addresing_mode(addr_mode);
		}
	}

	operation_type decode_bit_op(instruction inst)
	{
		auto data = decode_immediate<std::uint8_t>(inst);
		switch (data >> 6)
		{
		case 0b01:
			return operation_type::tst_bit_at;
		case 0b11:
			return operation_type::set_bit_at;
		case 0b10:
			return operation_type::res_bit_at;
		case 0b00:
		{
			std::uint8_t op = (data & 0b00111000) >> 3;
			switch (op)
			{
			case 0b000:
				return operation_type::rlc_at;
			case 0b001:
				return operation_type::rrc_at;
			case 0b010:
				return operation_type::rl_at;
			case 0b011:
				return operation_type::rr_at;
			case 0b100:
				return operation_type::sla_at;
			case 0b101:
				return operation_type::sra_at;
			case 0b110:
				return operation_type::sll_at;
			case 0b111:
				return operation_type::srl_at;
			}
		}
		}

		throw std::runtime_error("decode_bit_op internal error: unreachable code");
	}

	std::int8_t& decode_bit_reg(std::uint8_t bit_reg)
	{
		switch (bit_reg)
		{
		case register_type::A:
			return regs.main_set.A;
		case register_type::B:
			return regs.main_set.B;
		case register_type::C:
			return regs.main_set.C;
		case register_type::D:
			return regs.main_set.D;
		case register_type::E:
			return regs.main_set.E;
		case register_type::H:
			return regs.main_set.H;
		case register_type::L:
			return regs.main_set.L;
		default:
			throw std::runtime_error("decode_bit_reg error, unknown register " + su::bin_str(bit_reg));
		}
	}

	void advance_pc(instruction inst)
	{
		auto addressing_mode_size = [](addressing_mode addr_mode) -> std::uint16_t
		{
			switch (addr_mode)
			{
			case addressing_mode::immediate:
			case addressing_mode::indexed_ix:
			case addressing_mode::indexed_iy:
			case addressing_mode::immediate_bit:
				return 1;
			case addressing_mode::immediate_ext:
				return 2;
			default:
				return 0;
			}
		};

		std::uint16_t inst_size = inst.opcodes[1] == 0x0 ? 1 : 2;
		auto src_size = addressing_mode_size(inst.source);
		auto dest_size = addressing_mode_size(inst.destination);

		regs.PC += inst_size + src_size + dest_size;
	}

private:
	/* helper methods */
	static bool is_indexed(addressing_mode addr_mode)
	{
		return addr_mode == addressing_mode::indexed_ix || addr_mode == addressing_mode::indexed_iy;
	}
private:
	z80::memory& mem;
	z80::cpu_registers& regs;
};

}

#endif // __DECODER_HPP__
