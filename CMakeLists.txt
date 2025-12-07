cmake_minimum_required(VERSION 3.11)

set(GAME_TARGET "empires")
set(GAME_VERSION "0.1")

set(CMAKE_C_STANDARD		99)
set(CMAKE_CXX_STANDARD		17)

project(${GAME_TARGET}
	VERSION ${GAME_VERSION}
	LANGUAGES C CXX
)

option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_TESTS_HEADLESS "Only run headless unit tests" OFF)

option(BUILD_PROFILER "Use Tracy Profiler" OFF)

set(TEST_TARGET "testempires")
set(SERVER_TARGET "dedicated_server")

if (BUILD_PROFILER)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DTRACY_ENABLE=1")
endif()

if (BUILD_TESTS_HEADLESS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_TESTS_HEADLESS=1")
endif()

if (UNIX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
endif()

# dependencies

include(FetchContent)

find_package(SDL2 REQUIRED)
if (UNIX)
	find_package(PkgConfig)
	pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)
else()
	find_package(SDL2_mixer CONFIG REQUIRED)
endif()
find_package(OpenGL REQUIRED)

if (BUILD_PROFILER)
add_subdirectory(tracy)
endif()

if(BUILD_TESTS)
enable_testing()

find_package(GTest CONFIG REQUIRED)
endif()

if (WIN32)
	set(GAME_LIBRARIES
		${SDL2_LIBRARIES}
		$<IF:$<TARGET_EXISTS:SDL2_mixer::SDL2_mixer>,SDL2_mixer::SDL2_mixer,SDL2_mixer::SDL2_mixer-static>
		${OPENGL_LIBRARIES}
	)
else()
	set(GAME_LIBRARIES
		${SDL2_LIBRARIES}
		${SDL2_MIXER_LIBRARIES}
		${OPENGL_LIBRARIES}
	)
endif()

# source tree config

set(PROJECT_SRCDIR ${CMAKE_CURRENT_SOURCE_DIR})
set(GAME_DIR ${PROJECT_SRCDIR}/game)
set(GAME_SRCDIR ${GAME_DIR}/src)

file(GLOB_RECURSE GAME_SOURCES ${GAME_SRCDIR}/*.cpp ${GAME_SRCDIR}/*.c)

set(IMGUI_DIR ${PROJECT_SRCDIR}/imgui)
set(IMGUI_BACKENDS_DIR ${IMGUI_DIR}/backends)
file(GLOB IMGUI_SRC ${IMGUI_DIR}/*.cpp ${IMGUI_BACKENDS_DIR}/imgui_impl_opengl3.cpp ${IMGUI_BACKENDS_DIR}/imgui_impl_sdl2.cpp)

if(BUILD_TESTS)
set(TEST_DIR ${GAME_DIR}/tests)
file(GLOB TEST_SRC ${TEST_DIR}/*.cpp)
endif()

# fake epoll support for windows
if (WIN32)
set(WEPOLL_DIR ${PROJECT_SRCDIR}/wepoll)
set(WEPOLL_SRC ${WEPOLL_DIR}/wepoll.c)
endif()

set(GAME_SRC ${GAME_DIR}/main.cpp ${GAME_SOURCES} ${IMGUI_SRC} ${WEPOLL_SRC})

set(GAME_INCLUDE_DIRS ${GAME_SRCDIR}/core ${GAME_SRCDIR}/external)

set(LOCAL_INCLUDE_DIRS
	${IMGUI_DIR} ${IMGUI_BACKENDS_DIR}
	${WEPOLL_DIR}
	${PROJECT_SRCDIR}/tracy/public
	${PROJECT_SRCDIR}/miniz
)

set(SERVER_SRC ${GAME_DIR}/server.cpp ${GAME_SOURCES} ${IMGUI_SRC} ${WEPOLL_SRC})

# set recipes

add_executable(${GAME_TARGET} ${GAME_SRC})
add_executable(${SERVER_TARGET} ${SERVER_SRC})

if(BUILD_TESTS)
add_executable(${TEST_TARGET} ${GAME_SOURCES} ${IMGUI_SRC} ${WEPOLL_SRC} ${TEST_SRC})
endif()

# configure header and linker info

target_include_directories(${GAME_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${LOCAL_INCLUDE_DIRS}
	${GAME_INCLUDE_DIRS}
)

target_include_directories(${SERVER_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${LOCAL_INCLUDE_DIRS}
	${GAME_INCLUDE_DIRS}
)

if (BUILD_PROFILER)
target_link_libraries(${GAME_TARGET} ${GAME_LIBRARIES} TracyClient)
target_link_libraries(${SERVER_TARGET} ${GAME_LIBRARIES} TracyClient)
else()
target_link_libraries(${GAME_TARGET} ${GAME_LIBRARIES})
target_link_libraries(${SERVER_TARGET} ${GAME_LIBRARIES})
endif()

if(BUILD_TESTS)
target_include_directories(${TEST_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${LOCAL_INCLUDE_DIRS}
	${GAME_INCLUDE_DIRS}
)

target_link_libraries(${TEST_TARGET} PRIVATE ${GAME_LIBRARIES} GTest::gtest GTest::gtest_main)
endif()

# some mvsc magic. will be ignored on other platforms

set_target_properties(${GAME_TARGET} PROPERTIES
	VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SRCDIR}"
)

add_compile_definitions(
	"$<$<CONFIG:DEBUG>:_DEBUG>"
)

# disable BS warnings

if (WIN32)
	target_compile_definitions(${GAME_TARGET} PRIVATE
		_CRT_SECURE_NO_WARNINGS # no snprintf_s BS warnings
	)
	if (BUILD_TESTS)
		target_compile_definitions(${TEST_TARGET} PRIVATE
			_CRT_SECURE_NO_WARNINGS # no snprintf_s BS warnings
		)
	endif()
endif()

set(APP_PARENT_DIR "$<TARGET_FILE_DIR:${GAME_TARGET}>")

# copy DLLs

if(WIN32)
	foreach(DLL ${SDL2_DLLS})
		add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${APP_PARENT_DIR})
	endforeach()
endif()
