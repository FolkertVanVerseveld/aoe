# Tools overview

*NOTE this is currently being changed and it doesn't compile without some hacks*

*This is primarly for intended for developers and (eventually) modders*

To safe you a lot of time and pain to reverse engineer and understand the inner
workings of the genie game engine and its file formats, we have written these
tools.

These tools include:

Name    | Purpose
--------|-------------------------------------------------------------------
convert | Dump binary compatible source file
pestat  | Dump standard portable executable members and parameters
drs     | Dump assets while traversing a data resource set(=drs)
str     | Extract strings as ASCII data ignoring file format or empty strings
rsrc    | Fetches string data from portable executables
slp     | Dump slp info
wave    | Creates one audio playlist and loops each sample indefinitely

## Convert

This quick and dirty simple program can exactly replicate a file by generating a
hexdump that you can feed to e.g. NASM to reconstruct the file.

Example:

  ./convert data/Border.drs

The resulting file is written to ``a.out``. *It overwrites ``a.out`` by default*

## PEstat

Dump statistics for windoze Portable Executables (PE).

Example:

  ./pestat Empires.exe

The interface it is using is going to be integrated in the genie game engine,
because the engine needs to be able to parse string data in the .rsrc section in
windoze dlls for localization.

## DRS

Writes all assets it finds in a Data Resource Set (=DRS) named XXXXX.type where
type is one of ``wave shp slp binary`` and XXXXX is the resource identifier.

Example:

  ./drs data/Interfac.drs

## str

Dump all strings to stdout as if they were encoded in ASCII from a dll. It does
not properly handle all strings in the game because some characters are not in
ASCII. This tool is properly going to be rewritten.

## rsrc

Similar to str, but tries to really traverse the PE file honoring the PE file
format. It is not fully compliant because this format is tricky to get right in
plain C...

## slp

Dumps very basic info about the specified slp. Unfortunately, it is very
incomplete because our knowledge about this file format is very limited.

## wave

Easy to use audio playback tool that plays all audio files from a drs
indefinitely until enter is pressed. On windows, you need to compile OpenAL.
Since CreativeLabs doesn't provide the source themselves, you have to resort to
third party remakes like
[openal-soft](https://github.com/kcat/openal-soft/releases).

Typical compilation is similar to:

```
gcc -Wall -Wextra -pedantic -std=c99 -g -DDEBUG -I"C:/path/to/openal-soft/include/" -L"C:/path/to/openal-soft/build/" wave.c -o wave -lz -lOpenAL32
```
