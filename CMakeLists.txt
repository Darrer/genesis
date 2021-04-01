cmake_minimum_required (VERSION 2.8.11)
project (genesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")

set(SRC
    main.cpp
    ROM.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

set(HEADERS
    ROM.h
    ROM_debug.hpp
)

add_custom_target(
        format
        clang-format -style=file -i ${SRC} ${HEADERS}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
