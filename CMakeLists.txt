cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

project(terminal-audio VERSION 1.0.0)

include(FetchContent)

find_package(Curses REQUIRED)

set(KISSFFT_STATIC ON)
set(KISSFFT_TEST OFF)
set(KISSFFT_TOOLS OFF)

FetchContent_Declare(
    kissfft
    GIT_REPOSITORY https://github.com/mborgerding/kissfft
    GIT_TAG 131.1.0
)

FetchContent_MakeAvailable(kissfft)

include_directories(
    ${CURSES_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio
)

add_library(miniaudio
    miniaudio/miniaudio.c
)

add_executable(terminal-audio 
    src/main.cpp
    src/frontend.cpp
    src/audio.cpp
    src/utils.cpp
)


target_link_libraries(terminal-audio 
    ${CURSES_LIBRARIES}
    kissfft::kissfft
    miniaudio
)
