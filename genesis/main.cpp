#include <iostream>
#include <string_view>

#include "rom.h"
#include "smd/smd.h"
#include "rom_debug.hpp"
#include "string_utils.hpp"
#include "time_utils.h"

#include "sdl/palette_display.h"
#include "sdl/plane_display.h"

using namespace genesis;


template<class T>
void measure_and_log(const T& func, std::string_view msg)
{
	auto ms = time::measure_in_ms(func);
	std::cout << "Executing " << msg << " took " << ms << " ms\n";
}

void print_usage(const char* prog_path)
{
	std::cout << "Usage ./" << prog_path << " <path to rom>" << std::endl;
}

std::vector<std::unique_ptr<sdl::displayable>> create_displays(smd& smd)
{
	// TODO: interface between vdp/smd is not established yet, so use vdp::render directly

	std::vector<std::unique_ptr<sdl::displayable>> displays;

	displays.push_back(
		std::make_unique<sdl::palette_display>(smd.vdp().cram())
	);

	using vdp::impl::plane_type;

	displays.push_back(
		std::make_unique<sdl::plane_display>("plane a",
			[&smd]() { return smd.vdp().render().plane_width_in_pixels(plane_type::a); },
			[&smd]() { return smd.vdp().render().plane_hight_in_pixels(plane_type::a); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_plane_row(genesis::vdp::impl::plane_type::a, row_number, buffer); }
		)
	);

	displays.push_back(
		std::make_unique<sdl::plane_display>("plane b",
			[&smd]() { return smd.vdp().render().plane_width_in_pixels(plane_type::b); },
			[&smd]() { return smd.vdp().render().plane_hight_in_pixels(plane_type::b); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_plane_row(genesis::vdp::impl::plane_type::b, row_number, buffer); }
		)
	);

	displays.push_back(
		std::make_unique<sdl::plane_display>("sprites",
			[&smd]() { return smd.vdp().render().sprite_width_in_pixels(); },
			[&smd]() { return smd.vdp().render().sprite_height_in_pixels(); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_sprite_row(row_number, buffer); }
		)
	);

	return displays;
}

int main(int args, char* argv[])
{
	if(args != 2)
	{
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	try
	{
		std::string_view rom_path = argv[1];

		std::cout << "Going to parse: " << rom_path << std::endl;
		genesis::rom rom(rom_path);

		std::ostream& os = std::cout;
		genesis::debug::print_rom_header(os, rom.header());
		// os << "Actual checksum: " << su::hex_str(rom.checksum()) << std::endl;

		genesis::debug::print_rom_vectors(os, rom.vectors());
		// genesis::debug::print_rom_body(os, rom.body());

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cerr << "Cannot initialize SDL: " << SDL_GetError() << std::endl;
			return EXIT_FAILURE;
		}

		genesis::smd smd(rom_path);

		auto displays = create_displays(smd);

		smd.vdp().on_frame_end([&displays]()
		{
			measure_and_log([&]()
			{
				for(auto& disp: displays)
					disp->update();

				SDL_Event e;
				while(SDL_PollEvent(&e) > 0)
				{
					for(auto& disp: displays)
						disp->handle_event(e);
				}
			}, "update displays");
		});

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
