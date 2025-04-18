cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

project(terminal-audio VERSION 1.0.0)

find_package(Curses REQUIRED)
include_directories(
    ${CURSES_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio
    ${CMAKE_CURRENT_SOURCE_DIR}/kissfft
)

add_library(miniaudio miniaudio/miniaudio.c)
add_library(kissfft 
    kissfft/kiss_fft.c
    kissfft/kiss_fftr.c
)

add_executable(terminal-audio 
    src/main.cpp
    src/graph.cpp
    src/audio.cpp
)
target_link_libraries(terminal-audio 
    ${CURSES_LIBRARIES}
    miniaudio
    kissfft
)
