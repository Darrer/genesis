cmake_minimum_required (VERSION 2.8.11)
project (genesis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-Wall -Wextra")

set(SRC
    main.cpp
    ROM.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
