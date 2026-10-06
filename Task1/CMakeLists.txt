cmake_minimum_required(VERSION 3.20)
project(mafia_simulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

configure_file(config.yaml config.yaml COPYONLY)

find_package(CURL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(mafia
    src/main.cpp
    src/Player.cpp
    src/Roles.cpp
    src/Moderator.cpp
    src/Game.cpp
    src/Logger.cpp
    src/Config.cpp
    src/AiAgent.cpp
)
target_include_directories(mafia PRIVATE include)
target_link_libraries(mafia PRIVATE CURL::libcurl nlohmann_json::nlohmann_json)

if (MSVC)
    target_compile_options(mafia PRIVATE /W4)
else()
    target_compile_options(mafia PRIVATE -Wall -Wextra -Wpedantic)
    find_package(Threads REQUIRED)
    target_link_libraries(mafia PRIVATE Threads::Threads)
endif()
