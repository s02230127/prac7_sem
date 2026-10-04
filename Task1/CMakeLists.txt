cmake_minimum_required(VERSION 3.20)
project(mafia_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mafia
    src/main.cpp
)
target_include_directories(mafia PRIVATE include)

if (MSVC)
    target_compile_options(mafia PRIVATE /W4)
else()
    target_compile_options(mafia PRIVATE -Wall -Wextra -Wpedantic)
    find_package(Threads REQUIRED)
    target_link_libraries(mafia PRIVATE Threads::Threads)
endif()
