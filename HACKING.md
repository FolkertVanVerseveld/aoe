# Contributing

Contributions in any shape or form are very much appreciated. Development has
been on/off for many years, but there's always big and small tasks that need to
be done. Progress is tracked on the github
[issue tracker](https://github.com/FolkertVanVerseveld/aoe/issues) as much as
possible. Some issues are grouped as projects to mark milestone to work towards
to.

It greatly helps if you are experienced in C++ and OpenGL programming, but
there's also a lot of work to be done on documenting, playtesting etc.

## Documentation

Most documentation involving the source code is located in the source itself and
documentation about unit statistics, cheat codes etc. is documented in doc/.

Game engine and windows API specific file formats are discussed in doc/ with
pointers to external sources. We have written tools for some of these file
formats that you can find in the Tools section.

## Coding style

At the moment, the coding style is a bit of a mess. We are improving the coding
style and refactoring the code bit by bit as we fix and implement new things.
The coding style tends to follow many principles from the linux kernel coding
style. The best way to know how the coding style is implemented is by looking at
the source.

See also [this document](CODINGSTYLE.md) for more details.

## Libraries and dependencies

### Windows

This project is configured using [vcpkg](https://github.com/microsoft/vcpkg)
and built in Visual Studio. Install vcpkg, follow their instructions to
bootstrap. It is recommended to run:

  .\vcpkg integrate install

such that Visual Studio can find any packages you install using vcpkg. Then,
install the packages:

  .\vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-mixer-mpg123

For the unit tests, also install:

  .\vcpkg install gtest

To disable tests, disable the BUILD_TESTS property in CMakeLists.txt. To only
build HEADLESS tests, enable BUILD_TESTS_HEADLESS.

### Ubuntu and variants

These packages are required for development on Xubuntu:

Name              | Description
------------------|----------------------------------------
libsdl2-dev       | SDL2 development package
libsdl2-image-dev | SDL2 graphics development package
libsdl2-mixer-dev | SDL2 audio development package

A typical installation of these packages would be:

  sudo apt-get install gcc make pkg-config libsdl2-{,image-,mixer-}dev

Then, compiling should be as simple as:

  mkdir build && cd build
  cmake ../
  make

It may also work in parallel (not tested):
  make -j8

The following commands are expected to be installed
(descriptions are taken from their respective manuals):

Name        | Description
------------|--------------------------------------------------
pkg-config  | Return metainformation about installed libraries
gcc         | GNU project C and C++ compiler
git         | the stupid content tracker
make        | GNU make utility to maintain groups of programs
cmake       | CMake Command-Line Reference

# Reverse engineering

For compatibilty, some inner workings from the engine (e.g. assets loading and
parsing) have to be simulated. Some sections of the game and other related files
are dissassembled in reverse_engineering/ for research purposes.

The documentation is really tricky to get right here, so it may be misleading or
plainly wrong. Feel free to look around and make your own improvements. We use
the IDA unsupported free version (v8.2) from Hex-Rays for analyzing the code.

You can download your own free copy at [their website](https://www.hex-rays.com/products/ida/support/download_freeware.shtml).
*This version is free for non-commercial use only!*
