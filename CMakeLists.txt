cmake_minimum_required(VERSION 3.11)

set(GAME_TARGET "empires")
set(GAME_VERSION "0.1")

set(CMAKE_C_STANDARD		11)
set(CMAKE_CXX_STANDARD		17)

project(${GAME_TARGET}
	VERSION ${GAME_VERSION}
	LANGUAGES C CXX
)

option(BUILD_TESTS "Build unit tests" ON)

set(TEST_TARGET "testempires")

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
	${PROJECT_SRCDIR}/json/single_include
	${PROJECT_SRCDIR}/miniz
)

# set recipes

add_executable(${GAME_TARGET} ${GAME_SRC})

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

target_link_libraries(${GAME_TARGET} ${GAME_LIBRARIES})

if(BUILD_TESTS)
target_include_directories(${TEST_TARGET} PRIVATE
	${SDL2_INCLUDE_DIRS}
	${OPENGL_INCLUDE_DIR}
	${LOCAL_INCLUDE_DIRS}
	${GAME_INCLUDE_DIRS}
)

target_link_libraries(${TEST_TARGET} PRIVATE ${GAME_LIBRARIES} GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
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
endif()

set(APP_PARENT_DIR "$<TARGET_FILE_DIR:${GAME_TARGET}>")

# copy DLLs

if(WIN32)
	foreach(DLL ${SDL2_DLLS})
		add_custom_command(TARGET ${GAME_TARGET} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DLL} ${APP_PARENT_DIR})
	endforeach()
endif()
