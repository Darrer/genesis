#ifndef __SDL_INPUT_DEVICE_H__
#define __SDL_INPUT_DEVICE_H__

#include <array>
#include <map>
#include <type_traits>

#define SDL_MAIN_HANDLED
#include "io_ports/input_device.h"

#include <SDL2/SDL.h>

namespace genesis::sdl
{

static const std::map<int /* SDLK */, io_ports::key_type> default_key_layout = {
	{SDLK_TAB, io_ports::key_type::START},

	{SDLK_w, io_ports::key_type::UP},	   {SDLK_s, io_ports::key_type::DOWN}, {SDLK_a, io_ports::key_type::LEFT},
	{SDLK_d, io_ports::key_type::RIGHT},

	{SDLK_k, io_ports::key_type::A},	   {SDLK_l, io_ports::key_type::B},	   {SDLK_SPACE, io_ports::key_type::C},
};

class input_device : public io_ports::input_device
{
public:
	input_device(std::map<int /* SDLK */, io_ports::key_type> key_layout = default_key_layout) : key_layout(key_layout)
	{
		key_pressed.fill(false);
	}

	bool is_key_pressed(io_ports::key_type key) override
	{
		auto key_number = io_ports::key_type_index(key);
		return key_pressed.at(key_number);
	};

	void handle_event(const SDL_Event& event)
	{
		switch(event.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP: {
			SDL_Keycode key = event.key.keysym.sym;
			bool pressed = event.type == SDL_KEYDOWN;

			if(key_layout.contains(key))
			{
				auto key_idx = io_ports::key_type_index(key_layout[key]);
				key_pressed.at(key_idx) = pressed;
			}
		}
		}
	}

private:
	std::map<int /* SDLK */, io_ports::key_type> key_layout;
	std::array<bool, io_ports::key_type_count> key_pressed;
};

} // namespace genesis::sdl

#endif // __SDL_INPUT_DEVICE_H__
