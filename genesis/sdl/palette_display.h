#ifndef __SDL_PALETTE_DISPLAY_H__
#define __SDL_PALETTE_DISPLAY_H__

#include "base_display.h"
#include "vdp/memory.h"
#include "vdp/output_color.h"


namespace genesis::sdl
{

class palette_display : public base_display
{
public:
	palette_display(vdp::cram_t& cram) : base_display("palette", 600, 200), cram(cram)
	{
	}

	void update() override
	{
		if(m_window == nullptr)
		{
			// window was destroyed, nothing to do
			return;
		}

		auto* screenSurface = SDL_GetWindowSurface(m_window);
		SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 0xFF, 0xFF, 0xFF));

		int x = 0;
		int y = 0;
		for(unsigned palette = 0; palette < 4; ++palette)
		{
			y = (palette * 35) + 50;
			for(unsigned col = 0; col < 16; ++col)
			{
				x = (col * 35) + 5;
				genesis::vdp::output_color color = cram.read_color(palette, col);

				SDL_Rect rect = {x, y, 25, 25}; // x, y, width, height
				SDL_FillRect(screenSurface, &rect, SDL_MapRGB(screenSurface->format, 
					color.red * 36, color.green * 36, color.blue * 36));
			}
		}

		SDL_UpdateWindowSurface(m_window);
	}

private:
	vdp::cram_t& cram;
};

}

#endif // __GENESIS_SDL_PALETTE_DISPLAY_H__
