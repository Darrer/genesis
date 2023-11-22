#ifndef __SDL_DISPLAYABLE_H__
#define __SDL_DISPLAYABLE_H__

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

namespace genesis::sdl
{

class displayable
{
public:
	virtual ~displayable() = default;

	virtual void update() = 0;
	virtual void handle_event(const SDL_Event& event) = 0;
};

}

#endif // __SDL_DISPLAYABLE_H__
