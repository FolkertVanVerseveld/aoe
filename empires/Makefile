.PHONY: default clean

CC?=gcc
CFLAGS=-Wall -Wextra -pedantic -Wno-parentheses -Wno-implicit-fallthrough -std=gnu11
LDLIBS=
LDFLAGS?=
DEBUG_LDLIBS?=
DEBUG_LDFLAGS?=

CXX?=g++
CXXFLAGS=-Wall -Wextra -pedantic -Wno-parentheses -Wno-implicit-fallthrough -std=c++11

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

UI_LDLIBS=$(shell pkg-config --cflags --libs sdl2 gl openal) -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2_gfx

default: empires
empires: empires.o ui.o game.o qtree.o fs.o drs.o gfx.o sfx.o ../setup/res.o ../setup/def.o
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) empires.o ui.o game.o qtree.o fs.o drs.o gfx.o sfx.o ../setup/res.o ../setup/def.o -o empires $(LDFLAGS) $(LDLIBS) $(UI_LDLIBS) -lpe $(shell pkg-config --cflags --libs openssl) -lstdc++
empires.o: empires.c cfg.h
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) empires.c -c -o empires.o $(LDFLAGS) $(LDLIBS)
game.o: game.hpp game.cpp ui.h image.hpp render.hpp world.hpp
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) game.cpp -c -o game.o $(LDFLAGS) $(LDLIBS)
ui.o: ui.h ui.cpp game.hpp drs.h image.hpp render.hpp
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) ui.cpp -c -o ui.o $(LDFLAGS) $(LDLIBS)
qtree.o: world.hpp qtree.cpp
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) qtree.cpp -c -o qtree.o $(LDFLAGS) $(LDLIBS)
gfx.o: gfx.h gfx.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) gfx.c -c -o gfx.o $(LDFLAGS) $(LDLIBS)
sfx.o: sfx.h sfx.cpp cfg.h
	$(CXX) $(CXXFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) sfx.cpp -c -o sfx.o $(LDFLAGS) $(LDLIBS)
fs.o: fs.h fs.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) fs.c -c -o fs.o $(LDFLAGS) $(LDLIBS)
drs.o: drs.h drs.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) drs.c -c -o drs.o $(LDFLAGS) $(LDLIBS)
../setup/res.o: ../setup/res.h ../setup/res.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) ../setup/res.c -c -o ../setup/res.o $(LDFLAGS) $(LDLIBS)
../setup/def.o: ../setup/def.h ../setup/def.c
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) ../setup/def.c -c -o ../setup/def.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f empires *.o
