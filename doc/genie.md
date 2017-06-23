# Genie game engine

The Genie game engine has been in development since at least 1997 while the
original Age of Empires was being developed. The engine has undergone multiple
revisions in Rise of Rome and the sequels.

All data files are custom formatted archives which mostly contain linked lists
to data blocks which contain the raw resource data. These files are suffixed
'drs'. We will call them Data Resource Set or just drs file when we refer to a
drs file.

For a simple freestanding implementation that can extract wave files while
traversing drs files, see tools/drs.c

## Data Resource Set

A set of files which is just a simple archive that has a single linked list for
each category of files. The supported types are:

Internal name | Type/Description
--------------|-------------------------
wave          | audio
shp           | vector shape
slp           | Pro/ENGINEER 3d graphics
bina          | everything else

## Audio

All audio files in the drs files use wave codec ranging from 8kHz to 22050Hz in
either mono or stereo.

## Model graphics

A graphics is composed of (I think) some vector shapes and a 'slp' file which
defines the pixel data and the shapes are for collision checking.

## Other binaries

This section name is misleading since it also contains JASC palettes, which are
just plain text files. This is probably the 'everything else' section.

# Trivia

It looks like gold was the first resource that has been implemented. This is
based on the fact that the first string item in the string table is gold. The
order in which they appear is: gold, wood, food, stone.
