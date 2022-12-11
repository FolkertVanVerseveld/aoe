cmake_minimum_required(VERSION 3.7)

project(AoE)

# Set to TRUE to use tracy profiler framework
set(PROFILING FALSE)

if(PROFILING)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRACY_ENABLE=1")
	set(PROFILE_SOURCES "../tracy/public/TracyClient.cpp")
endif()

find_package(SDL2 REQUIRED)
if(UNIX)
	message(STATUS "using unix")
	find_package(PkgConfig REQUIRED)
	pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)
	pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)
	find_package(Threads)
else()
	find_package(SDL2_image REQUIRED)
	find_package(SDL2_mixer REQUIRED)
endif()
find_package(OpenGL REQUIRED)

file(GLOB_RECURSE AOE_SRC "src/*.cpp" "../wepoll/wepoll.c")
file(GLOB AOE_TEST "tests/*.cpp")

file(GLOB IMGUI_CORE_SRC "../imgui/*.cpp")
set(IMGUI_BACKEND_SRC "../imgui/backends/imgui_impl_sdl.cpp" "../imgui/backends/imgui_impl_opengl3.cpp")
set(IMGUI_SRC ${IMGUI_CORE_SRC} ${IMGUI_BACKEND_SRC})

file(GLOB AOE_DLLS "*.dll")
file(COPY ${AOE_DLLS} DESTINATION ${CMAKE_BINARY_DIR})

include_directories("../imgui/" "../imgui/backends" ${SDL2_INCLUDE_DIRS} "../wepoll/" "../tracy/public")

add_executable(empires "main.cpp" ${AOE_SRC} ${IMGUI_SRC} ${PROFILE_SOURCES})
add_executable(testempires ${AOE_TEST} ${AOE_SRC} ${IMGUI_SRC} ${PROFILE_SOURCES})

target_link_libraries(empires ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${OPENAL_LIBRARY} ${SDL2_MIXER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(testempires ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${OPENGL_LIBRARIES} ${OPENAL_LIBRARY} ${SDL2_MIXER_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
