cmake_minimum_required (VERSION 3.0)
project(tests)

include(FetchContent)

FetchContent_Declare(
	googletest

	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.12.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
	json

	GIT_REPOSITORY https://github.com/nlohmann/json.git
	GIT_TAG v3.11.2
)

FetchContent_MakeAvailable(googletest json)


add_executable(${GENESIS_TESTS})
target_sources(${GENESIS_TESTS}
PRIVATE
	endian.cpp
	rom.cpp
	memory.cpp
	helper.hpp

	z80/cpu_registers.cpp
	z80/tap_loader.hpp
	z80/tests_runner.cpp

	m68k/test_case.h
	m68k/tests_loader.h
	m68k/tests_loader.cpp
	m68k/tests_runner.cpp
)

target_link_libraries(${GENESIS_TESTS} gtest_main)
target_link_libraries(${GENESIS_TESTS} nlohmann_json::nlohmann_json)

target_link_libraries(${GENESIS_TESTS} ${GENESIS_LIB})

include(GoogleTest)
gtest_discover_tests(${GENESIS_TESTS})

# copy z80 exercisers
file(COPY z80/exercisers/zexdoc DESTINATION z80)
file(COPY z80/exercisers/zexall DESTINATION z80)
file(COPY z80/exercisers/z80full.tap DESTINATION z80)
file(COPY z80/exercisers/z80doc.tap DESTINATION z80)
