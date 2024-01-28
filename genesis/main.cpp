#include <iostream>
#include <iomanip>
#include <string_view>
#include <filesystem>

#include "rom.h"
#include "smd/smd.h"
#include "rom_debug.hpp"
#include "string_utils.hpp"
#include "time_utils.h"

#include "sdl/palette_display.h"
#include "sdl/plane_display.h"
#include "sdl/input_device.h"

using namespace genesis;


void print_usage(const char* prog_path)
{
	std::wcout << "Usage ." << std::filesystem::path::preferred_separator
		<< prog_path << " <path to rom>" << std::endl;
}

void print_key_layout(const std::map<int /* SDLK */, io_ports::key_type>& layout)
{
	std::cout << "==== Key layout ====" << std::endl;

	const auto keys = {
		io_ports::key_type::UP,
		io_ports::key_type::DOWN,
		io_ports::key_type::LEFT,
		io_ports::key_type::RIGHT,
		io_ports::key_type::START,
		io_ports::key_type::MODE,
		io_ports::key_type::A,
		io_ports::key_type::B,
		io_ports::key_type::C,
		io_ports::key_type::X,
		io_ports::key_type::Y,
		io_ports::key_type::Z,
	};

	for(auto key : keys)
	{
		auto it = std::find_if(layout.cbegin(), layout.cend(),
			[key](const auto& el) { return el.second == key; });

		if(it == layout.cend())
			continue;

		auto sdl_key = it->first;
		std::cout << std::setw(5) << io_ports::key_type_name(key)
			<< " -> " << SDL_GetKeyName(sdl_key) << std::endl;
	}

	std::cout << "====================" << std::endl;
}

template<class T>
void measure_and_log(const T& func, std::string_view msg)
{
	auto ms = time::measure_in_ms(func);
	std::cout << "Executing " << msg << " took " << ms << " ms\n";
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

	displays.push_back(
		std::make_unique<sdl::plane_display>("active display",
			[&smd]() { return smd.vdp().render().active_display_width(); },
			[&smd]() { return smd.vdp().render().active_display_height(); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_active_display_row(row_number, buffer); }
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

		std::cout << "Parsing: " << rom_path << std::endl;
		genesis::rom rom(rom_path);

		genesis::debug::print_rom_header(std::cout, rom.header());
		// os << "Actual checksum: " << su::hex_str(rom.checksum()) << std::endl;

		std::cout << "ROM vector table:" << std::endl;
		genesis::debug::print_rom_vectors(std::cout, rom.vectors());
		// genesis::debug::print_rom_body(os, rom.body());

		if (SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cerr << "Cannot initialize SDL: " << SDL_GetError() << std::endl;
			return EXIT_FAILURE;
		}

		print_key_layout(sdl::default_key_layout);
		auto input_device = std::make_shared<sdl::input_device>();

		genesis::smd smd(rom_path, input_device);

		auto displays = create_displays(smd);

		smd.vdp().on_frame_end([&]()
		{
			// measure_and_log([&]()
			// {
				for(auto& disp: displays)
					disp->update();

				SDL_Event e;
				while(SDL_PollEvent(&e) > 0)
				{
					for(auto& disp: displays)
						disp->handle_event(e);
					input_device->handle_event(e);
				}
			// }, "render frame");
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
