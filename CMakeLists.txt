cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(curses-tutorial VERSION 1.0.0)

find_package(Curses REQUIRED)
include_directories(
    ${CURSES_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio
)

add_library(graph src/graph.cpp)
add_library(miniaudio miniaudio/miniaudio.c)

add_executable(curses-tutorial src/main.cpp)
target_link_libraries(curses-tutorial 
    ${CURSES_LIBRARIES}
    graph
    miniaudio
)
