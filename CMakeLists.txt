cmake_minimum_required(VERSION 3.30)
project(sdl2_project CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(sdl2_project main.cpp)
target_link_libraries(sdl2_project ${SDL2_LIBRARIES})