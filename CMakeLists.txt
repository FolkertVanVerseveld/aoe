cmake_minimum_required (VERSION 2.8)

project (main)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
	add_definitions("-DLINUX=1")
	find_package(PkgConfig REQUIRED)
	find_package(Threads)
	pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
endif()

find_package(SDL2 REQUIRED)
if (NOT LINUX)
	find_package(SDL2_MIXER REQUIRED)
endif()
find_package(OpenGL REQUIRED)

if(LINUX)
	set(INCLUDE_WINDOWS "")
else()
	set(INCLUDE_WINDOWS "libs/windows")
endif()

# https://cmake.org/cmake/help/v3.0/command/file.html
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")# "libs/gl3w/GL/gl3w.c")

include_directories(${SDL2_INCLUDE_DIRS} ${INCLUDE_WINDOWS})# "libs/gl3w")
add_executable (main ${SOURCES})# ${GL3W_SOURCES})
target_link_libraries(main ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
