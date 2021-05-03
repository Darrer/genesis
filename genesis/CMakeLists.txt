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
	endian.hpp
	memory.hpp

	z80/cpu_registers.hpp
	z80/cpu.h
	z80/cpu.cpp

	z80/impl/cpu_unit.hpp
	z80/impl/base_unit.hpp
	z80/impl/arithmetic_logic_unit.hpp
)

# executable based on core lib
add_executable(${GENESIS})
target_sources(${GENESIS}
PRIVATE
	main.cpp
)

target_link_libraries(${GENESIS} PRIVATE ${GENESIS_LIB})

