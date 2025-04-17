cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

project(curses-tutorial VERSION 1.0.0)

find_package(Curses REQUIRED)
include_directories(
    ${CURSES_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio
)

add_library(miniaudio miniaudio/miniaudio.c)

add_executable(curses-tutorial 
    src/main.cpp
    src/graph.cpp
    src/audio.cpp
)
target_link_libraries(curses-tutorial 
    ${CURSES_LIBRARIES}
    miniaudio
)
