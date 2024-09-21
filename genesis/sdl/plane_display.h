#ifndef __GENESIS_SDL_PLANE_DISPLAY_H__
#define __GENESIS_SDL_PLANE_DISPLAY_H__

#include <string_view>
#include <functional>
#include <stdexcept>
#include <cstdint>
#include <array>
#include <span>

#include "base_display.h"
#include "vdp/output_color.h"

#include "time_utils.h"

#include <iostream>
#include <iomanip>

namespace genesis::sdl
{

// create active_display class to render VDP Active Display?

// This class responsible for displaying:
// - Plane A/B/W
// - Sprites
class plane_display : public base_display
{
public:
	using get_width_func = std::function<int()>;
	using get_height_func = std::function<int()>;

	using row_buffer = std::span<vdp::output_color>;
	using get_row_func = std::function<row_buffer(unsigned /* row number */, row_buffer /* input buffer */)>;

public:
	plane_display(std::string_view title, get_width_func get_width, get_height_func get_height, get_row_func get_row)
	{
		if(get_width == nullptr)
			throw std::invalid_argument("get_width");
		if(get_height == nullptr)
			throw std::invalid_argument("get_height");
		if(get_row == nullptr)
			throw std::invalid_argument("get_height");
		
		m_get_width = get_width;
		m_get_height = get_height;
		m_get_row = get_row;

		m_width = get_width();
		m_height = get_height();

		m_title = title;

		create_window(title, m_width, m_height);
		m_renderer = SDL_CreateRenderer(m_window, -1, 0);
		SDL_RendererInfo rendererInfo;
		if(SDL_GetRendererInfo(m_renderer, &rendererInfo) == 0)
		{
			std::cout << "Renderer name for window '" << title << "' : " << rendererInfo.name << ". Flags:";
			std::cout << " SOFTWARE=" << !!(rendererInfo.flags & SDL_RENDERER_SOFTWARE);
			std::cout << " ACCELERATED=" << !!(rendererInfo.flags & SDL_RENDERER_ACCELERATED);
			std::cout << " PRESENTVSYNC=" << !!(rendererInfo.flags & SDL_RENDERER_PRESENTVSYNC);
			std::cout << " TARGETTEXTURE=" << !!(rendererInfo.flags & SDL_RENDERER_TARGETTEXTURE);
			std::cout << std::endl;
		}
		else
		{
			std::cerr << "Could not get renderer info! SDL_Error: " << SDL_GetError() << std::endl;
		}
		m_texture = create_texture(m_width, m_height);
	}

	~plane_display()
	{
		SDL_DestroyRenderer(m_renderer);
		SDL_DestroyTexture(m_texture);
	}

	void update() override
	{
		if(m_window == nullptr)
		{
			// window was destroyed, nothing to do
			return;
		}

		int curr_width = m_get_width();
		int curr_height = m_get_height();

		// For some reason we have to re-create texture if width/height are changed
		if(curr_width != m_width || curr_height != m_height)
		{
			m_width = curr_width;
			m_height = curr_height;

			SDL_DestroyTexture(m_texture);
			m_texture = create_texture(m_width, m_height);
		}

		/* auto ns =  */time::measure_in_ns([&]()
		{
			int pixel_pos = 0;
			for(int row_number = 0; row_number < m_height; ++row_number)
			{
				auto row = m_get_row(row_number, buffer);
				for(auto color : row)
				{
					// TODO: use alpha
					std::uint32_t pix = 0x0;
					pix |= color.red << 21;
					pix |= color.green << 13;
					pix |= color.blue << 5;
					pixels.at(pixel_pos++) = pix;
				}
			}
		});

		// double ms = (double)ns / 1'000'000;
		// std::cout << "Forming " << m_title << " frame took " << std::setprecision(3) << ms << " ms\n";

		SDL_SetWindowSize(m_window, m_width, m_height);
		SDL_UpdateTexture(m_texture, nullptr, pixels.data(), 4 * m_width);
		SDL_RenderClear(m_renderer);
		SDL_RenderCopy(m_renderer, m_texture, nullptr, nullptr);
		SDL_RenderPresent(m_renderer);
	}

private:
	SDL_Texture* create_texture(int width, int height)
	{
		return SDL_CreateTexture(m_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, width, height);
	}

private:
	// Intentionally use static buffer for all plane_display instances as:
	// - we do not support multithreading redraw
	// - we use these buffers only to immediately copy the result to SDL

	// Assume there cannot be more then 1024 pixels per row
	static std::array<genesis::vdp::output_color, 1024> buffer;

	// Assume there cannot be more then 1024x1024 pixels
	static std::array<std::uint32_t, 1048576> pixels;

private:
	get_width_func m_get_width;
	get_height_func m_get_height;
	get_row_func m_get_row;

	SDL_Renderer* m_renderer;
	SDL_Texture* m_texture;

	int m_width;
	int m_height;

	std::string m_title;
};

}

#endif // __GENESIS_SDL_PLANE_DISPLAY_H__
