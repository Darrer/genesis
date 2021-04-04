cmake_minimum_required (VERSION 3.0)
project (genesis)

set(SRC
    main.cpp
    rom.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

set(HEADERS
    rom.h
    rom_debug.hpp
    string_utils.hpp
    endianness.hpp
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
