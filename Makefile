.PHONY: default all clean

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Wno-parentheses -std=gnu11 $(shell pkg-config --cflags xtcommon)
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

SDL_LDLIBS = $(shell pkg-config --cflags --libs sdl2) -lSDL2_image -lSDL2_ttf

default: setup
all: setup
setup: setup.o
	$(CC) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) setup.o -o setup $(LDFLAGS) $(LDLIBS) $(SDL_LDLIBS) -lpe $(shell pkg-config --cflags --libs openssl)

clean:
	rm -f setup *.o
