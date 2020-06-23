cmake_minimum_required (VERSION 2.8)

project (main)

set(CMAKE_CXX_STANDARD 17)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

if(LINUX)
	set(INCLUDE_WINDOWS "")
else()
	set(INCLUDE_WINDOWS "libs/windows")
endif()

# https://cmake.org/cmake/help/v3.0/command/file.html
file(GLOB_RECURSE SOURCES "src/*.cpp" "libs/gl3w/GL/gl3w.c")

include_directories(${SDL2_INCLUDE_DIRS} ${INCLUDE_WINDOWS} "libs/gl3w")
add_executable (main ${SOURCES} ${GL3W_SOURCES})
target_link_libraries(main ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES})
