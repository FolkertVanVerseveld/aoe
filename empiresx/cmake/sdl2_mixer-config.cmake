set(prefix "${CMAKE_CURRENT_LIST_DIR}")
set(exec_prefix "${prefix}")
set(SDL2_MIXER_PREFIX "${prefix}")
set(SDL2_MIXER_EXEC_PREFIX "${prefix}")

set(SDL2_MIXER_INCLUDE_DIRS "${prefix}/include")

if(${CMAKE_SIZEOF_VOID_P} MATCHES 8)
  set(libdir "${prefix}/lib/x64")
  set(SDL2_MIXER_LIBDIR "${prefix}/lib/x64")
else()
  set(libdir "${prefix}/lib/x86")
  set(SDL2_MIXER_LIBDIR "${prefix}/lib/x86")
endif()

set(SDL2_MIXER_LIBRARIES "${SDL2_MIXER_LIBDIR}/SDL2_mixer.lib")
string(STRIP "${SDL2_MIXER_LIBRARIES}" SDL2_MIXER_LIBRARIES)
