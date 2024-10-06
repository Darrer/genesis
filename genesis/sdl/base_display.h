#ifndef __SDL_BASE_DISPLAY_H__
#define __SDL_BASE_DISPLAY_H__

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <string_view>
#include <stdexcept>
#include <iostream>

#include "displayable.h"


namespace genesis::sdl
{

class base_display : public displayable
{
public:
	base_display() : m_window(nullptr)
	{
	}

	base_display(std::string_view title, int width, int height) : m_window(nullptr)
	{
		create_window(title, width, height);
	}

	~base_display()
	{
		destroy_window();
	}

	void handle_event(const SDL_Event& event) override
	{
		if(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE
			&& event.window.windowID == SDL_GetWindowID(m_window))
		{
			destroy_window();
		}
	}

	bool is_closed() const override { return m_window == nullptr; }

protected:
	void create_window(std::string_view title, int width, int height)
	{
		destroy_window();

		// in most cases we need a simple window with the same characteristics
		m_window = SDL_CreateWindow(
					title.data(),
					SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
					width, height,
					SDL_WINDOW_SHOWN
					);

		if(m_window == nullptr)
			throw std::runtime_error("Cannot create window: " + std::string(SDL_GetError()));
	}

	void destroy_window()
	{
		if(m_window != nullptr)
		{
			SDL_DestroyWindow(m_window);
			m_window = nullptr;
		}
	}

protected:
	SDL_Window* m_window;
};

}

#endif // __SDL_BASE_DISPLAY_H__
