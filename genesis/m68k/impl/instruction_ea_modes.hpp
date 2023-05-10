#ifndef __M68K_INSTRUCTION_EA_MODES_HPP__
#define __M68K_INSTRUCTION_EA_MODES_HPP__

#include "ea_decoder.hpp"
#include "instruction_type.h"

namespace genesis::m68k::impl
{

struct instruction_ea_modes
{
	template<class T>
	constexpr instruction_ea_modes(inst_type inst, const T& modes)
	// constexpr instruction_ea_modes(inst_type inst, const std::initializer_list<addressing_mode>& modes)
	{
		this->inst = inst;
		supported_modes.fill(addressing_mode::unknown);

		std::size_t i = 0;
		for(auto mode : modes)
			supported_modes.at(i++) = mode;
	}

	inst_type inst;
	std::array<addressing_mode, 12> supported_modes;
};

using mode = addressing_mode;

static constexpr mode all[] = { mode::data_reg, mode::addr_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc, mode::imm };

static constexpr mode data_alterable[] = { mode::data_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

static constexpr mode data[] = { mode::data_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc, mode::imm };

static constexpr mode data_except_imm[] = { mode::data_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc };

static constexpr mode alterable[] = { mode::data_reg, mode::addr_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

static constexpr mode memory_alterable[] = { mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

static constexpr mode control_alterable[] = { mode::indir, mode::predec, mode::disp_indir,
		mode::index_indir, mode::abs_short, mode::abs_long };

static constexpr mode control[] = { mode::indir, mode::disp_indir,
		mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc };

static constexpr mode postincrement[] = { mode::indir, mode::postinc, mode::disp_indir,
		mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc };

const constexpr instruction_ea_modes ea_modes[] =
{
	{ inst_type::ADDsrc, all },
	{ inst_type::ADDdst, memory_alterable },
	{ inst_type::ADDA, all },
	{ inst_type::ADDI, data_alterable },
	{ inst_type::ADDQ, alterable },

	{ inst_type::SUBsrc, all },
	{ inst_type::SUBdst, memory_alterable },
	{ inst_type::SUBA, all },
	{ inst_type::SUBI, data_alterable },
	{ inst_type::SUBQ, alterable },

	{ inst_type::ANDsrc, data },
	{ inst_type::ANDdst, memory_alterable },
	{ inst_type::ANDI, data_alterable },

	{ inst_type::ORsrc, data },
	{ inst_type::ORdst, memory_alterable },
	{ inst_type::ORI, data_alterable },

	{ inst_type::EOR, data_alterable },
	{ inst_type::EORI, data_alterable },

	{ inst_type::CMP, all },
	{ inst_type::CMPA, all },
	{ inst_type::CMPI, data_alterable },

	{ inst_type::NEG, data_alterable },
	{ inst_type::NEGX, data_alterable },

	{ inst_type::NOT, data_alterable },

	{ inst_type::MOVE, all }, // NOTE: only SRC operand is specified here
	{ inst_type::MOVEA, all },
	{ inst_type::MOVEtoCCR, data},
	{ inst_type::MOVEfromSR, data_alterable },
	{ inst_type::MOVEtoSR, data },
	{ inst_type::MOVEMtoMEM, control_alterable },
	{ inst_type::MOVEMtoREG, postincrement },

	{ inst_type::ASLRmem, memory_alterable },
	{ inst_type::ROLRmem, memory_alterable },
	{ inst_type::LSLRmem, memory_alterable },
	{ inst_type::ROXLRmem, memory_alterable },

	{ inst_type::TST, data_alterable },

	{ inst_type::CLR, data_alterable },

	{ inst_type::MULS, data },
	{ inst_type::MULU, data },
	{ inst_type::DIVS, data },
	{ inst_type::DIVU, data },

	{ inst_type::BTSTreg, data },
	{ inst_type::BTSTimm, data_except_imm },
	{ inst_type::BSETreg, data_alterable },
	{ inst_type::BSETimm, data_alterable },
	{ inst_type::BCLRreg, data_alterable },
	{ inst_type::BCLRimm, data_alterable },
	{ inst_type::BCHGreg, data_alterable },
	{ inst_type::BCHGimm, data_alterable },

	{ inst_type::JMP, control },
	{ inst_type::JSR, control },
	{ inst_type::LEA, control },
	{ inst_type::PEA, control },

	{ inst_type::CHK, data },

	{ inst_type::SCC, data_alterable },

	{ inst_type::NBCD, data_alterable },

	{ inst_type::TAS, data_alterable }
};

static constexpr std::array<addressing_mode, 8> movem_dest = { mode::data_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

}

#endif // __M68K_INSTRUCTION_EA_MODES_HPP__
