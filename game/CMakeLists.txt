cmake_minimum_required(VERSION 3.7)

project(AoE)

find_package(SDL2 REQUIRED)
if(UNIX)
	message(STATUS "using unix")
	find_package(PkgConfig REQUIRED)
	find_package(Threads)
endif()
find_package(OpenGL REQUIRED)

file(GLOB AOE_SRC "src/*.cpp")
file(GLOB AOE_TEST "tests/*.cpp")

file(GLOB AOE_DLLS "*.dll")
file(COPY ${AOE_DLLS} DESTINATION ${CMAKE_BINARY_DIR})

add_executable(empires "main.cpp" ${AOE_SRC})
add_executable(testempires ${AOE_TEST} ${AOE_SRC})

target_link_libraries(empires ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(testempires ${SDL2_LIBRARIES} ${OPENGL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
