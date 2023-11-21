cmake_minimum_required (VERSION 3.0)

project (genesis)

include(FetchContent)
# set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
# set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
FetchContent_Declare(
	SDL
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG release-2.28.5
	# GIT_TAG main # use SDL3::SDL3
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL)

# set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)

# library
add_library(${GENESIS_LIB})
target_sources(${GENESIS_LIB}
PRIVATE
	rom.cpp
	rom.h
	rom_debug.hpp

	string_utils.hpp
	exception.hpp
	endian.hpp
	memory.hpp

	smd/smd.h
	smd/smd.cpp
	smd/impl/m68k_interrupt_access.h

	memory/addressable.h
	memory/dummy_memory.h
	memory/memory_builder.h
	memory/memory_builder.cpp
	memory/dummy_memory.h
	memory/read_only_memory_unit.h

	z80/cpu_bus.hpp
	z80/cpu_registers.hpp
	z80/cpu.cpp
	z80/cpu.h
	z80/io_ports.hpp

	z80/impl/operations.hpp
	z80/impl/instructions.hpp
	z80/impl/decoder.hpp
	z80/impl/executioner.hpp
	z80/impl/inst_finder.hpp

	m68k/impl/bus_manager.h
	m68k/impl/bus_manager.cpp
	m68k/impl/prefetch_queue.hpp
	m68k/impl/opcode_decoder.cpp
	m68k/impl/bus_scheduler.cpp

	m68k/cpu_registers.hpp
	m68k/bus_access.h
	m68k/interrupting_device.h
	m68k/cpu.h
	m68k/cpu.cpp

	vdp/impl/fifo.h
	vdp/impl/render.h
	vdp/impl/render.cpp
	vdp/impl/name_table.h
	vdp/impl/sprite_table.h
	vdp/impl/plane_type.h
	vdp/impl/color.h
	vdp/vdp.h
	vdp/vdp.cpp
	vdp/registers.h
	vdp/register_set.h
	vdp/settings.h
	vdp/ports.h
	vdp/ports.cpp
	vdp/memory.h
	vdp/output_color.h
	vdp/m68k_bus_access.h
	vdp/m68k_interrupt_access.h
)

# executable based on core lib
add_executable(${GENESIS})
target_sources(${GENESIS}
PRIVATE
	main.cpp

	# SDL is not part of the core
	sdl/palette_display.h
	sdl/plane_display.h
	sdl/plane_display.cpp
)

# target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL3::SDL3)
# target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL2::SDL2)
target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL2::SDL2-static)
