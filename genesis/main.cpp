#include "rom.h"
#include "rom_debug.hpp"
#include "string_utils.hpp"
#include "time_utils.h"

#include "smd/smd.h"

#include <string_view>
#include <iostream>

#include "sdl/palette_display.h"
#include "sdl/plane_display.h"


template<class T>
void measure_and_log(const T& func, std::string_view msg)
{
	auto ms = genesis::time::measure_in_ms(func);
	std::cout << "Executing " << msg << " took " << ms << " ms\n";
}

void print_usage(char* prog_path)
{
	std::cout << "Usage ./" << prog_path << " <path to rom>" << std::endl;
}


int main(int args, char* argv[])
{
	if(args != 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	std::string rom_path = argv[1];

	try
	{
		std::cout << "Going to parse: " << rom_path << std::endl;
		genesis::rom rom(rom_path);

		std::ostream& os = std::cout;
		genesis::debug::print_rom_header(os, rom.header());
		os << "Actual checksum: " << su::hex_str(rom.checksum()) << std::endl;

		genesis::debug::print_rom_vectors(os, rom.vectors());
		// genesis::debug::print_rom_body(os, rom.body());
	}
	catch(std::exception& e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "Cannot initialize SDL: " << SDL_GetError() << std::endl;
		return EXIT_FAILURE;
	}

	try
	{
		genesis::smd smd(rom_path);

		auto& render = smd.vdp().render();

		genesis::sdl::palette_display palette_display(smd.vdp().cram());

		genesis::sdl::plane_display plane_a_display("plane a",
			[&render]() { return render.plane_width_in_pixels(genesis::vdp::impl::plane_type::a); },
			[&render]() { return render.plane_hight_in_pixels(genesis::vdp::impl::plane_type::a); },
			[&render](unsigned row_number, genesis::sdl::plane_display::row_buffer buffer)
				{ return render.get_plane_row(genesis::vdp::impl::plane_type::a, row_number, buffer); }
		);

		genesis::sdl::plane_display plane_b_display("plane b",
			[&render]() { return render.plane_width_in_pixels(genesis::vdp::impl::plane_type::b); },
			[&render]() { return render.plane_hight_in_pixels(genesis::vdp::impl::plane_type::b); },
			[&render](unsigned row_number, genesis::sdl::plane_display::row_buffer buffer)
				{ return render.get_plane_row(genesis::vdp::impl::plane_type::b, row_number, buffer); }
		);

		genesis::sdl::plane_display sprite_display("sprite",
			[&render]() { return render.sprite_width_in_pixels(); },
			[&render]() { return render.sprite_height_in_pixels(); },
			[&render](unsigned row_number, genesis::sdl::plane_display::row_buffer buffer)
				{ return render.get_sprite_row(row_number, buffer); }
		);


		auto on_frame_end = [&]()
		{
			measure_and_log([&]()
			{
				palette_display.redraw();
				palette_display.handle_events();

				plane_a_display.redraw();
				plane_a_display.handle_events();

				plane_b_display.redraw();
				plane_b_display.handle_events();

				sprite_display.redraw();
				sprite_display.handle_events();
			}, "redraw all displays");
		};

		smd.vdp().on_frame_end(on_frame_end);

		while(true) // Don't really care about timings so far
		{
			smd.cycle();
		}
	}
	catch(const std::invalid_argument& e)
	{
		std::cerr << "Invalid Argument exception: " << e.what() << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

	SDL_Quit();
	return EXIT_SUCCESS;
}
