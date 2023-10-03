cmake_minimum_required (VERSION 3.0)
project (genesis)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
	add_compile_options(/W3 /WX /O2)
else()
	add_compile_options(-Wall -Wextra -pedantic -O3 -fpermissive -Wno-switch)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(genesis)
include_directories(tests)

set(GENESIS genesis)
set(GENESIS_LIB ${GENESIS}_core)
set(GENESIS_TESTS ${GENESIS}_tests)


add_subdirectory(genesis)


enable_testing()

# TODO: make building tests optional
# if(BUILD_TESTING)
	add_subdirectory(tests)
# endif()


add_subdirectory(extra)
add_subdirectory(docker)
