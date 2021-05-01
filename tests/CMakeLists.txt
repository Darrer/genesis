cmake_minimum_required (VERSION 3.0)
project(tests)

include(FetchContent)
FetchContent_Declare(
	googletest

	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.10.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)


add_executable(${GENESIS_TESTS})
target_sources(${GENESIS_TESTS}
PRIVATE
	endian.cpp
	rom.cpp

	memory.cpp
	z80/cpu_registers.cpp
	z80/arithmetic_logic_unit.cpp
)

target_link_libraries(${GENESIS_TESTS} gtest_main)
target_link_libraries(${GENESIS_TESTS} ${GENESIS_LIB})

include(GoogleTest)
gtest_discover_tests(${GENESIS_TESTS})

