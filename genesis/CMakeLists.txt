cmake_minimum_required (VERSION 3.0)

project (genesis)

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

	m68k/impl/base_unit.h
	m68k/impl/base_unit.cpp
	m68k/impl/bus_manager.hpp
	m68k/impl/prefetch_queue.hpp
	m68k/impl/opcode_decoder.cpp
	m68k/impl/bus_scheduler.cpp

	m68k/cpu_registers.hpp
	m68k/cpu.h
	m68k/cpu.cpp

	vdp/vdp.h
	vdp/vdp.cpp
	vdp/registers.h
	vdp/register_set.h
	vdp/settings.h
)

# executable based on core lib
add_executable(${GENESIS})
target_sources(${GENESIS}
PRIVATE
	main.cpp
)

target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB})

