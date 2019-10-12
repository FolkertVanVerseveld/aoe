# Log

All important coding activities are documented here.

Sat, Oct 12, 2019 16:29:57

So right now, the engine uses SDL for cross-platform event handling stuff. While
this is great, we need additional stuff to get things working properly and
making sure everything works out-of-the-box, giving a plug and play feeling.

In order to resolve some cross-platform issues, we need the XenoTech common
library that provides workarounds for libc issues (I'm looking at you, windows!)
and provides posix stuff for non-posix platforms (e.g. mmap).

The genie/ folder provides two libraries: libgenie.a and libgeniegame.a. The
former is used in setup/ and tools/ where no graphical subsystem is desired or
available. The real game uses libgeniegame.a, because we want to move a lot of
UI stuff into the engine such that the game does not need to bother about them.

Currently, lots of code in empires/ really should be in genie/. Because the
complexity (or rather the maintainability) is not good, we need to rewrite large
parts to make sure it is fast, stable, robust and easy to use. While C++ sounds
nice in the beginning, I've come to realise that it creates more problems than
solves problems in my case because: I don't and probably won't consider myself
a good C++ programmer to begin with, I don't want to spend lots of time figuring
out how to get subtle C++ architectural problems right the first time and just
want to build stuff without double checking all possible optimisations and side
effects. Also, many components that are written in C++ don't have to be in C++,
because when I finally get to writing a public API for modding purposes, C++
will probably be a lot more painful to work with than a C interface.


Sat, Oct 12, 2019 16:44:26

Porting code from empires/ to genie/
Embed fonts into executable

Sat, Oct 12, 2019 18:53:10

Finally embedded fonts into the executable.
Still need to fix some minor things here and there.
