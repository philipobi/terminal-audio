cmake_minimum_required(VERSION 4.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
#set(CMAKE_BUILD_TYPE Debug)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

project(terminal-audio VERSION 1.0.0)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
#set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

include(FetchContent)

find_package(Curses REQUIRED)

set(KISSFFT_DATATYPE "float" CACHE BOOL "set fft data type" FORCE)
set(KISSFFT_STATIC ON CACHE BOOL "build static lib" FORCE)
set(KISSFFT_TEST OFF CACHE BOOL "dont build tests" FORCE)
set(KISSFFT_TOOLS OFF CACHE BOOL "dont build tools" FORCE)

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
)
target_link_libraries(terminal-audio 
    ${CURSES_LIBRARIES}
    kissfft::kissfft
    miniaudio
)
