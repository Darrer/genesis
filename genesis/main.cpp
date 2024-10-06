#include <iostream>
#include <string_view>
#include <filesystem>
#include <iomanip>

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
	std::cout << "==== Key layout ====\n";

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
		std::cout << std::setw(5) << io_ports::key_type_name(key) << " -> " << SDL_GetKeyName(sdl_key) << '\n';
	}

	std::cout << "====================\n";
}

template<class T>
void measure_and_log(T func, std::string_view msg)
{
	auto ms = time::measure_in_ms(func);
	std::cout << "Executing " << msg << " took " << ms << " ms\n";
}

std::vector<std::unique_ptr<sdl::displayable>> create_displays(smd& smd, std::string rom_title)
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
			[&smd]() { return smd.vdp().render().plane_height_in_pixels(plane_type::a); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_plane_row(genesis::vdp::impl::plane_type::a, row_number, buffer); }
		)
	);

	displays.push_back(
		std::make_unique<sdl::plane_display>("plane b",
			[&smd]() { return smd.vdp().render().plane_width_in_pixels(plane_type::b); },
			[&smd]() { return smd.vdp().render().plane_height_in_pixels(plane_type::b); },
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
		std::make_unique<sdl::plane_display>(rom_title,
			[&smd]() { return smd.vdp().render().active_display_width(); },
			[&smd]() { return smd.vdp().render().active_display_height(); },
			[&smd](unsigned row_number, sdl::plane_display::row_buffer buffer)
				{ return smd.vdp().render().get_active_display_row(row_number, buffer); }
		)
	);

	return displays;
}

std::string get_rom_title(const genesis::rom& rom)
{
	const auto& header = rom.header();
	if(!header.game_name_overseas.empty())
		return std::string{header.game_name_overseas};
	return std::string{header.game_name_domestic};
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

		std::cout << "Reading " << rom_path << '\n';
		genesis::rom rom(rom_path);

		genesis::debug::print_rom_header(std::cout, rom.header());
		if(rom.checksum() != rom.header().rom_checksum)
		{
			std::cout << "WARNING: ROM checksum mismatch (expected " << su::hex_str(rom.checksum())
				<< " vs actual " << su::hex_str(rom.header().rom_checksum) << ")\n";
		}

		if(SDL_Init(SDL_INIT_VIDEO) < 0)
		{
			std::cerr << "Cannot initialize SDL: " << SDL_GetError() << '\n';
			return EXIT_FAILURE;
		}

		print_key_layout(sdl::default_key_layout);
		auto input_device = std::make_shared<sdl::input_device>();

		std::string rom_title = get_rom_title(rom);

		genesis::smd smd(rom, input_device);

		auto displays = create_displays(smd, std::move(rom_title));

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

		const auto batch_cycles = 10'000'000ull;
		auto cycle = 0ull;

		auto start = std::chrono::high_resolution_clock::now();

		while(true) // Don't really care about timings so far
		{
			smd.cycle();
			++cycle;

			if(cycle == batch_cycles)
			{
				auto stop = std::chrono::high_resolution_clock::now();
				auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
				auto ns_per_cycle = dur / cycle;
				std::cout << "ns per cycle: " << ns_per_cycle.count() << '\n';

				start = stop;
				cycle = 0;

				bool all_closed = std::all_of(displays.cbegin(), displays.cend(),
					[](const auto& d) {return d->is_closed(); });
				if(all_closed)
				{
					break;
				}
			}
		}
	}
	catch(const std::invalid_argument& e)
	{
		std::cerr << "Invalid Argument exception: " << e.what() << '\n';
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

	SDL_Quit();
	return EXIT_SUCCESS;
}
