cmake_minimum_required (VERSION 3.0)
project (genesis)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
	# /w34189 - enable unused variable warnings
	# /w34100 - enable unused parameter warnings
	set(GENESIS_CXX_FLAGS /W3 /WX /O2 /w34189 /w34100)
	set(GENESIS_LINK_FLAGS)

	# list(APPEND GENESIS_CXX_FLAGS /DEBUG:FULL /Zi)
	# list(APPEND GENESIS_LINK_FLAGS /DEBUG:FULL)
else()
	set(GENESIS_CXX_FLAGS -Wall -Wextra -pedantic -O3 -fpermissive -Wno-switch)
	set(GENESIS_LINK_FLAGS)
endif()

message("Genesis CXX flags: ${GENESIS_CXX_FLAGS}")
message("Genesis link flags: ${GENESIS_LINK_FLAGS}")

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
