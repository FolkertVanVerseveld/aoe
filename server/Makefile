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
	DEBUG_LDLIBS += $(shell pkg-config --libs check)
	DEBUG_LDFLAGS += -fsanitize=address
endif
else
	CFLAGS += -O3
endif

default: server
all: server client
client: client.c
	$(CC) $(CFLAGS) -o client client.c $(shell pkg-config --cflags --libs ncurses) $(LDLIBS)
clean:
	rm -f server client *.o
