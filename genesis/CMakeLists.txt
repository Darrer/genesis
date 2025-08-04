cmake_minimum_required (VERSION 3.10)

project (genesis)

include(FetchContent)

# set(SDL_SHARED TRUE CACHE BOOL "Build a SDL shared library (if available)")
# set(SDL_STATIC TRUE CACHE BOOL "Build a SDL static library (if available)")
set(SDL_TEST FALSE CACHE BOOL "Do not build the SDL test library")
FetchContent_Declare(
	SDL
	GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
	GIT_TAG release-2.32.8
	# GIT_TAG main # use SDL3::SDL3
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(SDL)

# set_property(DIRECTORY "${CMAKE_BINARY_DIR}/_deps/sdl-src" PROPERTY EXCLUDE_FROM_ALL TRUE)

add_compile_options(${GENESIS_CXX_FLAGS})
add_link_options(${GENESIS_LINK_FLAGS})

# library
add_library(${GENESIS_LIB})
target_sources(${GENESIS_LIB}
PRIVATE
	smd/impl/m68k_bus_access.h
	smd/impl/m68k_interrupt_access.h
	smd/impl/z80_68bank.h
	smd/impl/z80_control_registers.h
	smd/impl/z80_io_ports.h

	smd/smd.cpp
	smd/smd.h
	
	memory/addressable.h
	memory/base_unit.h
	memory/dummy_memory.h
	memory/logging_memory.h
	memory/memory_builder.cpp
	memory/memory_builder.h
	memory/memory_unit.h
	memory/read_only_memory_unit.h

	z80/impl/decoder.hpp
	z80/impl/executioner.hpp
	z80/impl/inst_finder.hpp
	z80/impl/instructions.hpp
	z80/impl/operations.hpp

	z80/cpu_bus.hpp
	z80/cpu_registers.hpp
	z80/cpu.cpp
	z80/cpu.h
	z80/io_ports.hpp
	z80/memory.h

	m68k/impl/bus_manager.cpp
	m68k/impl/bus_manager.h
	m68k/impl/bus_scheduler.cpp
	m68k/impl/bus_scheduler.h
	m68k/impl/ea_decoder.hpp
	m68k/impl/ea_modes.h
	m68k/impl/exception_manager.h
	m68k/impl/exception_unit.hpp
	m68k/impl/instruction_type.h
	m68k/impl/instruction_unit.hpp
	m68k/impl/interrupt_riser.h
	m68k/impl/opcode_decoder.cpp
	m68k/impl/opcode_decoder.h
	m68k/impl/operations.hpp
	m68k/impl/pc_corrector.hpp
	m68k/impl/prefetch_queue.hpp
	m68k/impl/privilege_checker.hpp
	m68k/impl/size_type.h
	m68k/impl/timings.hpp
	m68k/impl/trace_riser.hpp

	m68k/bus_access.h
	m68k/cpu_bus.hpp
	m68k/cpu_registers.hpp
	m68k/interrupting_device.h
	m68k/cpu.h
	m68k/cpu.cpp

	vdp/impl/blank_flags.h
	vdp/impl/dma.h
	vdp/impl/fifo.h
	vdp/impl/hscroll_table.h
	vdp/impl/hv_counters.h
	vdp/impl/hv_unit.h
	vdp/impl/interrupt_unit.h
	vdp/impl/memory_access.h
	vdp/impl/name_table.h
	vdp/impl/plane_type.h
	vdp/impl/render.cpp
	vdp/impl/render.h
	vdp/impl/sprite_table.h
	vdp/impl/sprites_limits_tracker.h
	vdp/impl/vscroll_table.h

	vdp/control_register.h
	vdp/m68k_bus_access.h
	vdp/m68k_interrupt_access.h
	vdp/memory.h
	vdp/mode.h
	vdp/output_color.h
	vdp/ports.cpp
	vdp/ports.h
	vdp/read_buffer.h
	vdp/register_set.h
	vdp/registers.h
	vdp/settings.h
	vdp/vdp.cpp
	vdp/vdp.h

	io_ports/controller.h
	io_ports/disabled_port.h
	io_ports/input_device.h
	io_ports/key_type.h

	cpu_flags.hpp
	endian.hpp
	exception.hpp
	rom_debug.hpp
	rom.cpp
	rom.h
	string_utils.hpp
	time_utils.h
)

# executable based on core lib
add_executable(${GENESIS})
target_sources(${GENESIS}
PRIVATE
	# SDL is not part of the core
	sdl/base_display.h
	sdl/displayable.h
	sdl/input_device.h
	sdl/palette_display.h
	sdl/plane_display.cpp
	sdl/plane_display.h

	main.cpp
)

# target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL3::SDL3)
# target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL2::SDL2)
target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB} SDL2::SDL2-static)
