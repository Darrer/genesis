#ifndef __EXECUTIONER_HPP__
#define __EXECUTIONER_HPP__

#include "z80/cpu.h"
#include "string_utils.hpp"

#include "instructions.hpp"
#include "decoder.hpp"
#include "operations.hpp"
#include "inst_finder.hpp"


namespace genesis::z80
{

class executioner
{
public:
	executioner(z80::cpu& cpu) : cpu(cpu), dec(z80::decoder(cpu)), ops(z80::operations(cpu))
	{
	}

	void execute_one()
	{
		if(check_interrupts())
			return;

		if(cpu.bus().is_set(bus::RESET))
		{
			cpu.reset();
			return;
		}

		if(cpu.bus().is_set(bus::BUSREQ))
		{
			// accept request
			cpu.bus().set(bus::BUSACK);

			// asume setting BUSREQ will pause the CPU, should be good enough for our purposes
			return;
		}
		else
		{
			cpu.bus().clear(bus::BUSACK);
		}

		if(cpu.bus().is_set(bus::HALT))
		{
			// nothing to do
			return;
		}

		auto& mem = cpu.memory();
		auto& regs = cpu.registers();

		z80::opcode opcode = mem.read<z80::opcode>(regs.PC);
		z80::opcode opcode2 = mem.read<z80::opcode>(regs.PC + 1);

		auto inst = finder.fast_search(opcode, opcode2);
		exec_and_advance(inst);
	}

private:
	void exec_and_advance(z80::instruction inst)
	{
		exec(inst);
		if(need_advance_pc(inst.op_type))
			dec.advance_pc(inst);
	}

	void exec(z80::instruction inst)
	{
		interrupts_just_enabled = false;

		switch(inst.op_type)
		{
		/* 8-Bit Arithmetic Group */
		case operation_type::add:
			ops.add(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::adc:
			ops.adc(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sub:
			ops.sub(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::sbc:
			ops.sbc(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::and_8:
			ops.and_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::or_8:
			ops.or_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::xor_8:
			ops.xor_8(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::cp:
			ops.cp(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::inc_reg:
			ops.inc_reg(dec.decode_reg_8(inst.source));
			break;
		case operation_type::dec_reg:
			ops.dec_reg(dec.decode_reg_8(inst.source));
			break;
		case operation_type::inc_at:
			ops.inc_at(dec.decode_address(inst.source, inst));
			break;
		case operation_type::dec_at:
			ops.dec_at(dec.decode_address(inst.source, inst));
			break;

		/* 16-Bit Arithmetic Group */
		case operation_type::add_16:
			ops.add_16(dec.decode_reg_16(inst.source), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::adc_hl:
			ops.adc_hl(dec.decode_reg_16(inst.source));
			break;
		case operation_type::sbc_hl:
			ops.sbc_hl(dec.decode_reg_16(inst.source));
			break;
		case operation_type::inc_reg_16:
			ops.inc_reg_16(dec.decode_reg_16(inst.source));
			break;
		case operation_type::dec_reg_16:
			ops.dec_reg_16(dec.decode_reg_16(inst.source));
			break;

		/* 8/16-Bit Load Group */
		case operation_type::ld_reg:
			ops.ld_reg(dec.decode_byte(inst.source, inst), dec.decode_reg_8(inst.destination));
			break;
		case operation_type::ld_at:
			ops.ld_at(dec.decode_byte(inst.source, inst), dec.decode_address(inst.destination, inst));
			break;
		case operation_type::ld_16_at:
			ops.ld_at(dec.decode_2_bytes(inst.source, inst), dec.decode_address(inst.destination, inst));
			break;
		case operation_type::ld_ir:
			ops.ld_ir(dec.decode_reg_8(inst.source));
			break;
		case operation_type::ld_16_reg:
			ops.ld_reg(dec.decode_2_bytes(inst.source, inst), dec.decode_reg_16(inst.destination));
			break;
		case operation_type::ld_16_reg_from:
			ops.ld_reg_from(dec.decode_reg_16(inst.destination), dec.decode_address(inst.source, inst));
			break;
		case operation_type::push:
			ops.push(dec.decode_reg_16(inst.source));
			break;
		case operation_type::pop:
			ops.pop(dec.decode_reg_16(inst.destination));
			break;

		/* Call and Return Group */
		case operation_type::call:
			ops.call(dec.decode_address(inst.source, inst));
			break;
		case operation_type::call_cc:
			ops.call_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));
			break;
		case operation_type::rst:
			ops.rst(dec.decode_cc(inst));
			break;
		case operation_type::ret:
			ops.ret();
			break;
		case operation_type::reti:
			ops.reti();
			break;
		case operation_type::retn:
			ops.retn();
			break;
		case operation_type::ret_cc:
			ops.ret_cc(dec.decode_cc(inst));
			break;

		/* Jump Group */
		case operation_type::jp:
			ops.jp(dec.decode_2_bytes(inst.source, inst));
			break;
		case operation_type::jp_cc:
			ops.jp_cc(dec.decode_cc(inst), dec.decode_address(inst.source, inst));
			break;
		case operation_type::jr:
			ops.jr(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::jr_z:
			ops.jr_z(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::jr_nz:
			ops.jr_nz(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::jr_c:
			ops.jr_c(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::jr_nc:
			ops.jr_nc(dec.decode_byte(inst.source, inst));
			break;
		case operation_type::djnz:
			ops.djnz(dec.decode_byte(inst.source, inst));
			break;

		/* CPU Control Groups */
		case operation_type::nop:
			break;
		case operation_type::halt:
			ops.halt();
			break;
		case operation_type::di:
			ops.di();
			break;
		case operation_type::ei:
			ops.ei();
			interrupts_just_enabled = true;
			break;
		case operation_type::im0:
			ops.im0();
			break;
		case operation_type::im1:
			ops.im1();
			break;
		case operation_type::im2:
			ops.im2();
			break;

		/* Input and Output Group */
		case operation_type::out:
			ops.out(dec.decode_byte(inst.source, inst));
			break;

		/* Exchange, Block Transfer, and Search Group */
		case operation_type::ex_de_hl:
			ops.ex_de_hl();
			break;
		case operation_type::ex_af_afs:
			ops.ex_af_afs();
			break;
		case operation_type::exx:
			ops.exx();
			break;
		case operation_type::ex_16_at:
			ops.ex_16_at(dec.decode_reg_16(inst.source), dec.decode_address(inst.destination, inst));
			break;
		case operation_type::ldi:
			ops.ldi();
			break;
		case operation_type::ldir:
			ops.ldir();
			break;
		case operation_type::cpd:
			ops.cpd();
			break;
		case operation_type::cpdr:
			ops.cpdr();
			break;
		case operation_type::cpi:
			ops.cpi();
			break;
		case operation_type::cpir:
			ops.cpir();
			break;
		case operation_type::ldd:
			ops.ldd();
			break;
		case operation_type::lddr:
			ops.lddr();
			break;

		/* Rotate and Shift Group */
		case operation_type::rlca:
			ops.rlca();
			break;
		case operation_type::rrca:
			ops.rrca();
			break;
		case operation_type::rla:
			ops.rla();
			break;
		case operation_type::rra:
			ops.rra();
			break;
		case operation_type::rld:
			ops.rld();
			break;
		case operation_type::rrd:
			ops.rrd();
			break;
		case operation_type::rlc:
			ops.rlc(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::rlc_at:
			ops.rlc_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::rrc:
			ops.rrc(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::rrc_at:
			ops.rrc_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::rl:
			ops.rl(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::rl_at:
			ops.rl_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::rr:
			ops.rr(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::rr_at:
			ops.rr_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::sla:
			ops.sla(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::sla_at:
			ops.sla_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::sra:
			ops.sra(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::sra_at:
			ops.sra_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::srl:
			ops.srl(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::srl_at:
			ops.srl_at(dec.decode_address(inst.destination, inst));
			break;
		case operation_type::sll:
			ops.sll(dec.decode_reg_8(inst.destination));
			break;
		case operation_type::sll_at:
			ops.sll_at(dec.decode_address(inst.destination, inst));
			break;

		/* Bit Set, Reset, and Test Group */
		case operation_type::tst_bit:
			ops.tst_bit(dec.decode_byte(inst.destination, inst), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::tst_bit_at:
			ops.tst_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::set_bit:
			ops.set_bit(dec.decode_reg_8(inst.destination), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::set_bit_at:
			ops.set_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::res_bit:
			ops.res_bit(dec.decode_reg_8(inst.destination), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::res_bit_at:
			ops.res_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::bit_group:
			exec_bit_group(inst);
			break;

		/* General-Purpose Arithmetic */
		case operation_type::daa:
			ops.daa();
			break;
		case operation_type::cpl:
			ops.cpl();
			break;
		case operation_type::neg:
			ops.neg();
			break;
		case operation_type::ccf:
			ops.ccf();
			break;
		case operation_type::scf:
			ops.scf();
			break;

		default:
			throw std::runtime_error("exec_inst error: unsupported operation " + std::to_string(inst.op_type));
		}
	}

	void exec_bit_group(instruction inst)
	{
		std::uint8_t imm = dec.decode_immediate<std::uint8_t>(inst);

		std::uint8_t bin_reg = imm & 0b00000111;
		optional_reg_ref reg = std::nullopt;

		if(bin_reg != 0b110)
		{
			reg = dec.decode_bit_reg(bin_reg);
		}

		auto op_type = dec.decode_bit_op(inst);
		switch (op_type)
		{
		case operation_type::tst_bit_at:
			ops.tst_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst));
			break;
		case operation_type::set_bit_at:
			ops.set_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst), reg);
			break;
		case operation_type::res_bit_at:
			ops.res_bit_at(dec.decode_address(inst.destination, inst), dec.decode_bit(inst.source, inst), reg);
			break;

		case operation_type::rlc_at:
			ops.rlc_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::rrc_at:
			ops.rrc_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::rl_at:
			ops.rl_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::rr_at:
			ops.rr_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::sla_at:
			ops.sla_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::sra_at:
			ops.sra_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::srl_at:
			ops.srl_at(dec.decode_address(inst.destination, inst), reg);
			break;
		case operation_type::sll_at:
			ops.sll_at(dec.decode_address(inst.destination, inst), reg);
			break;
		default:
			throw std::runtime_error("exec_bit_group error: unsupported operation " + std::to_string(op_type));
		}
	}

	// TODO: move to decoder?
	bool need_advance_pc(operation_type op)
	{
		switch (op)
		{
		// these control PC itslef
		case operation_type::call:
		case operation_type::call_cc:
		case operation_type::rst:
		case operation_type::ret:
		case operation_type::reti:
		case operation_type::retn:
		case operation_type::ret_cc:
		case operation_type::jp:
		case operation_type::jp_cc:
		case operation_type::jr:
		case operation_type::jr_z:
		case operation_type::jr_nz:
		case operation_type::jr_c:
		case operation_type::jr_nc:
		case operation_type::ldir:
		case operation_type::cpdr:
		case operation_type::cpir:
		case operation_type::lddr:
		case operation_type::djnz:
			return false;
		default:
			return true;
		}
	}

	bool check_interrupts()
	{
		auto& bus = cpu.bus();

		if(bus.is_set(bus::BUSREQ))
		{
			// no interrupts if BUSREQ is set
			return false;
		}

		if(bus.is_set(bus::NMI))
		{
			ops.nonmaskable_interrupt();
			// TODO: should we clear NMI? Or somehow indicate interrupt is processing
			// otherwise we going to handle the same interrupt second time on the next cycle
			throw std::runtime_error("check_interrupts nonmaskable interrupts are not implmeneted properly");
			return true;
		}

		if(interrupts_just_enabled)
		{
			// we have to execute 1 instruction after enabling interrupts
			return false;
		}

		if(cpu.registers().IFF1 == 0)
		{
			// maskable interrupts are disabled
			return false;
		}
		
		if(bus.is_set(bus::INT))
		{
			// we had to accept interrupt first, then wait till get data,
			// but for simplicity assume data already on the bus
			exec_maskable_interrupt(bus.get_data());
			return true;
		}

		return false;
	}

	void exec_maskable_interrupt(std::uint8_t data)
	{
		switch (cpu.interrupt_mode())
		{
		case cpu_interrupt_mode::im0:
		{
			ops.maskable_interrupt_m0();
			instruction inst = finder.fast_search(data);
			exec(inst);
			break;
		}
		case cpu_interrupt_mode::im1:
			ops.maskable_interrupt_m1();
			break;
		case cpu_interrupt_mode::im2:
			ops.maskable_interrupt_m2(data);
			break;
		default:
			throw std::runtime_error("exec_maskable_interrupt internal error: unknown interrupt mode");
		}
	}

private:
	z80::cpu& cpu;
	z80::decoder dec;
	z80::operations ops;
	z80::inst_finder finder;
	bool interrupts_just_enabled = false;
};

}

#endif // __EXECUTIONER_HPP__
