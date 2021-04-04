cmake_minimum_required (VERSION 3.0)
project (genesis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

include_directories(genesis)

add_subdirectory(genesis)


enable_testing()

# if(BUILD_TESTING)
	add_subdirectory(tests)
# endif()
