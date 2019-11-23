.PHONY: default clean

FLAGS=-Wall -Wextra -pedantic -Wno-parentheses -Wno-implicit-fallthrough -I../include/ $(shell pkg-config --cflags xtcommon)

CC?=gcc
CFLAGS=$(FLAGS) -std=gnu11
LDLIBS=-lm -lz $(shell pkg-config --libs xtcommon)
LDFLAGS?=
DEBUG_LDLIBS?=
DEBUG_LDFLAGS?=

CXX?=g++
CXXFLAGS=$(FLAGS) -std=c++11

ifdef STRICT
	CFLAGS += -Werror
	CXXFLAGS += -Werror
endif
ifdef DEBUG
	CFLAGS += -g -DDEBUG
	CXXFLAGS += -g -DDEBUG
ifeq ($(DEBUG),2)
	CFLAGS += -fsanitize=address $(shell pkg-config --libs check)
	DEBUG_LDLIBS += $(shell pkg-config --libs check)
	DEBUG_LDFLAGS += -fsanitize=address
endif
else
	CFLAGS += -O3
endif

UI_LDLIBS=$(shell pkg-config --cflags --libs sdl2 gl openal) -lSDL2_image -lSDL2_mixer -lSDL2_gfx $(shell pkg-config --cflags --libs openssl)

ifdef CROSS_COMPILE
ifeq ($(CROSS_COMPILE),32)
	CC=i686-w64-mingw32-gcc
	CFLAGS += -I"/usr/local/i686-w64-mingw32/include/"

	CXX=i686-w64-mingw32-g++
	CXXFLAGS += -I"/usr/local/i686-w64-mingw32/include/"

	LDFLAGS += -L"/usr/local/lib" -L"/usr/local/i686-w64-mingw32/lib/"
else
	CC=x86_64-w64-mingw32-gcc
	CFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"

	CXX=x86_64-w64-mingw32-g++
	CXXFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"

	LDFLAGS += -L"/usr/local/lib" -L"/usr/local/x86_64-w64-mingw32/lib/"
endif

	UI_LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -mwin32 -lopengl32 -lstdc++ -lz

	CFLAGS += -I"/usr/local/include/"
	CXXFLAGS += -I"/usr/local/include/"
endif

LDLIBS += $(shell pkg-config --cflags xtcommon)

OBJECTS=empires.o \
	ui.o \
	editor.o \
	game.o \
	qtree.o \
	drs.o \
	cpn.o \
	scn.o \
	inflate.o \
	str.o \
	scenario.o \
	../lib/libgeniegame.a

default: empires
empires: $(OBJECTS)
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) $(OBJECTS) -o empires $(LDFLAGS) $(LDLIBS) $(UI_LDLIBS) -lstdc++
empires.o: empires.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) empires.c -c -o empires.o $(LDFLAGS) $(LDLIBS)

game.o: game.hpp game.cpp ui.h image.hpp render.hpp world.hpp drs.h lang.h
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) game.cpp -c -o game.o $(LDFLAGS) $(LDLIBS)

ui.o: ui.h ui.cpp game.o editor.o scenario.o math.h
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) ui.cpp -c -o ui.o $(LDFLAGS) $(LDLIBS)
editor.o: editor.cpp editor.hpp game.hpp world.hpp
scenario.o: scenario.cpp scenario.hpp cpn.h scn.h
inflate.o: inflate.cpp serialization.h
scn.o: scn.c inflate.o scn.h
str.o: str.c str.h
qtree.o: world.hpp qtree.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) editor.cpp -c -o editor.o $(LDFLAGS) $(LDLIBS)
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) qtree.cpp -c -o qtree.o $(LDFLAGS) $(LDLIBS)
cpn.o: cpn.h cpn.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) cpn.c -c -o cpn.o $(LDFLAGS) $(LDLIBS)
drs.o: drs.c drs.h res.h
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) drs.c -c -o drs.o $(LDFLAGS) $(LDLIBS)

../lib/libgeniegame.a:
	$(MAKE) -C ../genie/

../setup/def.o: ../setup/def.h ../setup/def.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) ../setup/def.c -c -o ../setup/def.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f empires *.o
