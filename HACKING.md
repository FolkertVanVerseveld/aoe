# Contributing

If you are really willing to help improve development, you can try to implement
missing stuff, start writing/improving documentation or fix a couple of bugs.
You have to grok basic stuff about POSIX and the C programming language to be
able to contribute. If you do not know what these things mean nor SDL2 or bash
then you will have a hard time with contributing.

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

Just download the official development and binary releases from libsdl.org
This project is compiled with CMake. It should 'just work' using Visual Studio.

### Ubuntu and variants

These packages are required for development on Xubuntu:

Name              | Description
------------------|----------------------------------------
libsdl2-dev       | SDL2 development package
libsdl2-image-dev | SDL2 graphics development package
libsdl2-mixer-dev | SDL2 audio development package

The following commands are expected to be installed
(descriptions are taken from their repsective manuals):

Name        | Description
------------|--------------------------------------------------
bash        | GNU Bourne-Again SHell
pkg-config  | Return metainformation about installed libraries
gcc         | GNU project C and C++ compiler
git         | the stupid content tracker
sed         | stream editor for filtering and transforming text
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
