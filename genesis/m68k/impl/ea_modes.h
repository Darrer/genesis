#ifndef __M68K_EA_MODES_H__
#define __M68K_EA_MODES_H__

namespace genesis::m68k::impl
{

/* 
static constexpr mode data_alterable[] = { mode::data_reg, mode::indir, mode::postinc, mode::predec,
	mode::disp_indir, mode::index_indir, mode::abs_short, mode::abs_long };

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
		mode::index_indir, mode::abs_short, mode::abs_long, mode::disp_pc, mode::index_pc }; */

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

}

#endif // __M68K_EA_MODES_H__
