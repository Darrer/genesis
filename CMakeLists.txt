cmake_minimum_required (VERSION 3.0.0)
project (genesis)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")

set(SRC
    src/main.cpp
    src/rom.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

set(HEADERS
    src/rom.h
    src/rom_debug.hpp
    src/string_utils.hpp
)

add_custom_target(
    format
    clang-format -style=file -i ${SRC} ${HEADERS}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

add_custom_target(
    tidy
    clang-tidy --quiet -header-filter=.* -p ${CMAKE_BINARY_DIR} ${SRC} |& sed "/\\d* warnings generated\\./d"
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
