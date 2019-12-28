.PHONY: all default clean

CC?=gcc
CFLAGS?=-Wall -Wextra -pedantic -std=c99
CFLAGS+=$(shell pkg-config --cflags xtcommon)
LDLIBS=$(shell pkg-config --libs xtcommon)

ifdef STRICT
	CFLAGS += -Werror
endif
ifdef DEBUG
	CFLAGS += -g -DDEBUG
ifeq ($(DEBUG),2)
	CFLAGS += -fsanitize=address $(shell pkg-config --libs check)
	LDLIBS += $(shell pkg-config --libs check)
	LDFLAGS += -fsanitize=address
endif
else
	CFLAGS += -O3
endif

default: server
all: server client

server: server.o net.o
client: client.o net.o
	$(CC) -o $@ $^ $(shell pkg-config --libs ncurses) $(LDLIBS)
client.o: client.c
	$(CC) -c -o $@ $(CFLAGS) $(DEBUG_LDFLAGS) $(shell pkg-config --cflags ncurses) $<
net.o: net.c common.h

clean:
	rm -f server client *.o
