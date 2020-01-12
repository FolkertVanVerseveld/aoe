cmake_minimum_required(VERSION 3.7)

project(empiresx)

if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

find_package(SDL2 REQUIRED)
if(LINUX)
	message(STATUS "using linux")
	find_package(PkgConfig REQUIRED)
	pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)
	pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
	pkg_search_module(SDL2_TTF REQUIRED SDL2_ttf)
else()
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_mixer REQUIRED)
	find_package(SDL2_ttf REQUIRED)
endif()
find_package(OpenGL REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})
file(GLOB SOURCES "*.cpp")
add_executable(empiresx ${SOURCES})

target_link_libraries(empiresx ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL2_MIXER_LIBRARIES} ${SDL2_TTF_LIBRARIES})
