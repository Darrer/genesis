cmake_minimum_required (VERSION 3.5)
project (genesis)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ENABLE_ADDRESS_SANITIZER "Enable AddressSanitizer" OFF)
option(ENABLE_MEMORY_SANITIZER "Enable MemorySanitizer" OFF)
option(ENABLE_UB_SANITIZER "Enable UndefinedBehaviorSanitizer" OFF)
option(INCLUDE_DEBUG_SYMBOLS "Include Debug Symbols" ON)
option(BUILD_TESTING "Build Tests" ON)

if (MSVC)
	# /w34189 - enable unused variable warnings
	# /w34100 - enable unused parameter warnings
	# /w34505 - enable unused function warnings
	set(GENESIS_CXX_FLAGS /W3 /WX /O2 /w34189 /w34100 /w34505)
	set(GENESIS_LINK_FLAGS)
else()
	set(GENESIS_CXX_FLAGS -Wall -Wextra -pedantic -O0 -fpermissive -Wno-switch)
	set(GENESIS_LINK_FLAGS)
endif()

# Set compiler flags for AddressSanitizer if enabled
if(ENABLE_ADDRESS_SANITIZER)
	if (MSVC)
		message(FATAL_ERROR "AddressSanitizer is not supported on MSVC.")
	else()
		list(APPEND GENESIS_CXX_FLAGS -fsanitize=address -fno-omit-frame-pointer -g)
		list(APPEND GENESIS_LINK_FLAGS -fsanitize=address)
	endif()
endif()

# Set compiler flags for MemorySanitizer if enabled
if(ENABLE_MEMORY_SANITIZER)
	if (MSVC)
		message(FATAL_ERROR "MemorySanitizer is not supported on MSVC.")
	else()
		# TODO: we need to build c++ std lib with memory sanitize flags
		list(APPEND GENESIS_CXX_FLAGS -fsanitize=memory -fPIE -fno-omit-frame-pointer -g)
		list(APPEND GENESIS_LINK_FLAGS -fsanitize=memory -pie)
	endif()
endif()

# Set compiler flags for UndefinedBehaviorSanitizer if enabled
if(ENABLE_UB_SANITIZER)
	if (MSVC)
		message(FATAL_ERROR "UndefinedBehaviorSanitizer is not supported on MSVC.")
	else()
		list(APPEND GENESIS_CXX_FLAGS -fsanitize=undefined)
		list(APPEND GENESIS_LINK_FLAGS -fsanitize=undefined)
	endif()
endif()

if(INCLUDE_DEBUG_SYMBOLS)
	if (MSVC)
		list(APPEND GENESIS_CXX_FLAGS /DEBUG:FULL /Zi)
		list(APPEND GENESIS_LINK_FLAGS /DEBUG:FULL)
	else()
		list(APPEND GENESIS_CXX_FLAGS -g -ggdb)
	endif()
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

if(BUILD_TESTING)
	enable_testing()
	add_subdirectory(tests)
endif()

add_subdirectory(extra)
add_subdirectory(docker)
