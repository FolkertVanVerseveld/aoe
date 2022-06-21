cmake_minimum_required(VERSION 3.7)

project(AoE)

# Set to TRUE to use tracy profiler framework
set(PROFILING FALSE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# Linux may not be detected properly
if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

add_subdirectory(game)
