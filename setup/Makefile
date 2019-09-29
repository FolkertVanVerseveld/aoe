# Copyright 2017-2019 Folkert van Verseveld. All rights reserved
.PHONY: default all clean

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Wno-parentheses -std=gnu11 -I../include/ $(shell pkg-config --cflags xtcommon)
LDLIBS=$(shell pkg-config --libs xtcommon)
LDFLAGS?=
DEBUG_LDLIBS?=
DEBUG_LDFLAGS?=

ifdef STRICT
	CFLAGS += -Werror
endif
ifdef DEBUG
	CFLAGS += -g -DDEBUG
ifeq ($(DEBUG),2)
	CFLAGS += -fsanitize=address $(shell pkg-config --libs check)
	DEBUG_LDLIBS += $(shell pkg-config --libs check)
	DEBUG_LDFLAGS += -fsanitize=address
endif
else
	CFLAGS += -O3
endif

SDL_LDLIBS = $(shell pkg-config --cflags --libs sdl2) -lSDL2_image -lSDL2_mixer -lSDL2_ttf

default: setup
all: setup bmp
setup: setup.o
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) setup.o ../lib/libgenie.a -o setup $(LDFLAGS) $(LDLIBS) $(SDL_LDLIBS)
bmp: bmp.o
	$(CC) $(CFLAGS) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) bmp.o ../lib/libgenie.a -o bmp $(LDFLAGS) $(LDLIBS)

clean:
	rm -f setup bmp *.o
