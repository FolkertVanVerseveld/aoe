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

ifdef CROSS_COMPILE
ifeq ($(CROSS_COMPILE),32)
	CC=i686-w64-mingw32-gcc
	CFLAGS += -I"/usr/local/i686-w64-mingw32/include/"

	CXX=i686-w64-mingw32-g++
	CXXFLAGS += -I"/usr/local/i686-w64-mingw32/include/"
else
	CC=x86_64-w64-mingw32-gcc
	CFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"

	CXX=x86_64-w64-mingw32-g++
	CXXFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"
endif
	CFLAGS += -I"/usr/local/include"
	CXXFLAGS += -I"/usr/local/include"
endif

OBJECTS=\
	def.o \
	gfx.o \
	res.o \
	fs.o \
	memory.o \
	pe.o

GAME_OBJECTS=\
	engine.o

ALL_OBJECTS=$(GAME_OBJECTS) $(OBJECTS)

default: ../lib/libgenie.a ../lib/libgeniegame.a

../lib:
	mkdir -p ../lib
../lib/libgenie.a: ../lib $(OBJECTS)
	ar -cr ../lib/libgenie.a $(OBJECTS)
../lib/libgeniegame.a: ../lib $(ALL_OBJECTS)
	ar -cr ../lib/libgeniegame.a $(ALL_OBJECTS)

clean:
	rm -f ../lib/libgenie.a ../lib/libgeniegame.a *.o
