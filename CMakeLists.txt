cmake_minimum_required (VERSION 2.8.11)
project (genesis)

set(CMAKE_CXX_STANDARD 14)

set(SRC
    main.cpp
)

add_executable(${PROJECT_NAME} ${SRC})
