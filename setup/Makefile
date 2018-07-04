.PHONY: default all clean

CC=gcc
CFLAGS=-Wall -Wextra -pedantic -Wno-parentheses -std=gnu11
LDLIBS=
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
all: setup bmp
setup: setup.o res.o def.o
	$(CC) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) setup.o res.o def.o -o setup $(LDFLAGS) $(LDLIBS) $(SDL_LDLIBS) -lpe $(shell pkg-config --cflags --libs openssl)
res.o: res.h res.c
	$(CC) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) res.c -c -o res.o $(LDFLAGS) $(LDLIBS)
def.o: def.h def.c
	$(CC) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) def.c -c -o def.o $(LDFLAGS) $(LDLIBS)
bmp: bmp.o
	$(CC) $(DEBUG_LDFLAGS) $(DEBUG_LDLIBS) bmp.o -o bmp $(LDFLAGS) $(LDLIBS)

clean:
	rm -f setup bmp *.o
