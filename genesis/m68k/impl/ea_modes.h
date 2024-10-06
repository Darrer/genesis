#ifndef __M68K_EA_MODES_H__
#define __M68K_EA_MODES_H__

#include "ea_decoder.hpp"

#include <algorithm>
#include <span>


namespace genesis::m68k::impl
{

enum class ea_modes
{
	none,
	all,

	/* all except Address Register Direct */
	data,

	/* data except Program Counter Indirect and Immediate */
	data_alterable,

	/* data except Immediate */
	data_except_imm,

	/* all except Program Counter Indirect and Immediate */
	alterable,

	/* alterable except Address & Data Register Direct */
	memory_alterable,

	/* all except Program Counter Indirect, Immediate and Register Direct */
	control,

	/* some specific modes used by MOVE command only */
	predecrement,
	postincrement
};


namespace modes
{

using mode = addressing_mode;

static constexpr const mode all[] = {mode::data_reg, mode::addr_reg,   mode::indir,		  mode::postinc,
									 mode::predec,	 mode::disp_indir, mode::index_indir, mode::abs_short,
									 mode::abs_long, mode::disp_pc,	   mode::index_pc,	  mode::imm};

static constexpr const mode data_alterable[] = {mode::data_reg,	  mode::indir,		 mode::postinc,	  mode::predec,
												mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long};

static constexpr const mode data[] = {mode::data_reg,	mode::indir,	   mode::postinc,	mode::predec,
									  mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long,
									  mode::disp_pc,	mode::index_pc,	   mode::imm};

static constexpr const mode data_except_imm[] = {mode::data_reg,   mode::indir,		  mode::postinc,   mode::predec,
												 mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long,
												 mode::disp_pc,	   mode::index_pc};

static constexpr const mode alterable[] = {mode::data_reg,	  mode::addr_reg,  mode::indir,
										   mode::postinc,	  mode::predec,	   mode::disp_indir,
										   mode::index_indir, mode::abs_short, mode::abs_long};

static constexpr const mode memory_alterable[] = {mode::indir,		 mode::postinc,	  mode::predec,	 mode::disp_indir,
												  mode::index_indir, mode::abs_short, mode::abs_long};

static constexpr const mode control[] = {mode::indir,	 mode::disp_indir, mode::index_indir, mode::abs_short,
										 mode::abs_long, mode::disp_pc,	   mode::index_pc};

static constexpr const mode predecrement[] = {mode::indir,		 mode::predec,	  mode::disp_indir,
											  mode::index_indir, mode::abs_short, mode::abs_long};

static constexpr const mode postincrement[] = {mode::indir,		mode::postinc,	mode::disp_indir, mode::index_indir,
											   mode::abs_short, mode::abs_long, mode::disp_pc,	  mode::index_pc};

} // namespace modes

static constexpr std::span<const addressing_mode> supported_modes(ea_modes modes)
{
	switch(modes)
	{
	case ea_modes::all:
		return modes::all;
	case ea_modes::data:
		return modes::data;
	case ea_modes::data_alterable:
		return modes::data_alterable;
	case ea_modes::data_except_imm:
		return modes::data_except_imm;
	case ea_modes::alterable:
		return modes::alterable;
	case ea_modes::memory_alterable:
		return modes::memory_alterable;
	case ea_modes::control:
		return modes::control;
	case ea_modes::predecrement:
		return modes::predecrement;
	case ea_modes::postincrement:
		return modes::postincrement;
	default:
		return {};
	}
}

static constexpr bool mode_is_supported(ea_modes modes, addressing_mode addr_mode)
{
	if(addr_mode == addressing_mode::unknown || modes == ea_modes::none)
		return false;

	auto supported = supported_modes(modes);
	return std::find(supported.begin(), supported.end(), addr_mode) != supported.end();
}

} // namespace genesis::m68k::impl

#endif // __M68K_EA_MODES_H__
