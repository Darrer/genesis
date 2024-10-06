#ifndef __VDP_IMPL_SPRITES_LIMITS_TRACKER_H__
#define __VDP_IMPL_SPRITES_LIMITS_TRACKER_H__

#include "sprite_table.h"
#include "vdp/settings.h"


namespace genesis::vdp::impl
{

// Track sprite line limits
class sprites_limits_tracker
{
public:
	sprites_limits_tracker(vdp::settings& sett) : sett(sett)
	{
		reset_limits();
	}

	bool line_limit_exceeded() const
	{
		return rendered_sprites >= sprites_per_line_limit() || rendered_pixels >= sett.display_width_in_pixels();
	}

	unsigned line_pixels_limit() const
	{
		unsigned max_pixels_per_line = sett.display_width_in_pixels();
		if(rendered_pixels >= max_pixels_per_line)
			return 0;
		return max_pixels_per_line - rendered_pixels;
	}

	void reset_limits()
	{
		rendered_sprites = 0;
		rendered_pixels = 0;
	}

	void on_sprite_draw(const sprite_table_entry& entry)
	{
		++rendered_sprites;
		rendered_pixels += (entry.horizontal_size + 1) * 8;
	}

private:
	unsigned sprites_per_line_limit() const
	{
		if(sett.display_width() == display_width::c40)
			return 20;
		return 16;
	}

private:
	vdp::settings& sett;
	unsigned rendered_sprites;
	unsigned rendered_pixels;
};

} // namespace genesis::vdp::impl

#endif // __VDP_IMPL_SPRITES_LIMITS_TRACKER_H__
