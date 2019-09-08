.PHONY: default clean

FLAGS=-Wall -Wextra -pedantic -Wno-parentheses -I../include/

CC?=gcc
CFLAGS=$(FLAGS) -std=c99

CXX?=g++
CXXFLAGS=$(FLAGS) -std=c++11

ifdef STRICT
	CFLAGS += -Werror
	CXXFLAGS += -Werror
endif
ifdef DEBUG
	CFLAGS += -g -DDEBUG
	CXXFLAGS += -g -DDEBUG
else
	CFLAGS += -O3
endif

OBJECTS=\
	def.o \
	res.o

default: ../lib/libgenie.a

../lib/libgenie.a: $(OBJECTS)
	mkdir -p ../lib
	ar -cr ../lib/libgenie.a $(OBJECTS)

clean:
	rm -f ../lib/libgenie.a *.o
