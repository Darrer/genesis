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

static constexpr mode alterable[] = { mode::data_reg, mode::addr_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

const constexpr instruction_ea_modes ea_modes[] =
{
	{ inst_type::ADD, all },
	{ inst_type::ADDA, all },
	{ inst_type::ADDI, data_alterable },
	{ inst_type::ADDQ, alterable },

	{ inst_type::ORI, data_alterable },
	{ inst_type::BTSTreg, data },
	// { inst_type::BTSTimm, }
};

}

#endif // __M68K_INSTRUCTION_EA_MODES_HPP__
