cmake_minimum_required (VERSION 3.10)
project(tests)

include(FetchContent)

add_compile_options(${MSAN_CXX_FLAGS})
add_link_options(${MSAN_LINK_FLAGS})

FetchContent_Declare(
	googletest

	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
	json

	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(googletest json)

add_compile_options(${GENESIS_CXX_FLAGS})
add_link_options(${GENESIS_LINK_FLAGS})

add_executable(${GENESIS_TESTS})
target_sources(${GENESIS_TESTS}
PRIVATE
	helpers/random.cpp
	helpers/random.h

	m68k/MCL/mcl.h
	m68k/MCL/runner.cpp

	m68k/OPCLOGR3/opcode_decoder.cpp
	m68k/OPCLOGR3/opcode_loader.cpp
	m68k/OPCLOGR3/opcode_loader.h

	m68k/performance/performance.cpp

	# Tom Harte's Tests
	m68k/THT/test_case.h
	m68k/THT/tests_loader.cpp
	m68k/THT/tests_loader.h
	m68k/THT/tests_runner.cpp

	m68k/THT_MAP/map_loader.cpp
	m68k/THT_MAP/map_loader.h
	m68k/THT_MAP/map_test.cpp

	m68k/bus_manager.cpp
	m68k/ea_decoder.cpp
	m68k/exception_unit.cpp
	m68k/int_dev.h
	m68k/prefetch_queue.cpp
	m68k/test_cpu.hpp
	m68k/test_program.h

	memory/helper.h
	memory/memory_builder.cpp
	memory/memory_unit.cpp

	vdp/blank_flags.cpp
	vdp/dma.cpp
	vdp/hv_counters.cpp
	vdp/ports.cpp
	vdp/render.cpp
	vdp/renderer_builder.hpp
	vdp/test_vdp.h

	z80/cpu_registers.cpp
	z80/tap_loader.hpp
	z80/tests_runner.cpp

	endian.cpp
	helper.hpp
	rom.cpp
)

target_link_libraries(${GENESIS_TESTS} gtest_main)
target_link_libraries(${GENESIS_TESTS} nlohmann_json::nlohmann_json)

target_link_libraries(${GENESIS_TESTS} ${GENESIS_LIB})

include(GoogleTest)
gtest_discover_tests(${GENESIS_TESTS} PROPERTIES DISCOVERY_TIMEOUT 60)

# copy z80 exercisers
file(COPY z80/exercisers/Spectrum/zexdoc DESTINATION z80)
file(COPY z80/exercisers/Spectrum/zexall DESTINATION z80)
file(COPY z80/exercisers/z80test/z80full.tap DESTINATION z80)
file(COPY z80/exercisers/z80test/z80doc.tap DESTINATION z80)

# copy m68k test files
file(COPY m68k/exercisers/OPCLOGR3.BIN DESTINATION m68k)
file(COPY m68k/exercisers/MCL/MC68000_test_all_opcodes.bin DESTINATION m68k)
file(COPY m68k/exercisers/THT/68000.official.json DESTINATION m68k)

file(GLOB THT_TESTS m68k/exercisers/THT/*.zip)
foreach(tst ${THT_TESTS})
	message(STATUS "extracting ${tst}")
	file(ARCHIVE_EXTRACT INPUT ${tst} DESTINATION m68k/v1)
endforeach()
