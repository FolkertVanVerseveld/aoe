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
	pe.o \
	fnt_data.o

GAME_OBJECTS=\
	engine.o \
	vfx.o \
	sfx.o

FONT_OBJECTS=\
	fnt_default.o \
	fnt_default_bold.o \
	fnt_button.o \
	fnt_large.o

FONTS=\
	fnt_default.png \
	fnt_default_bold.png \
	fnt_button.png \
	fnt_large.png

ALL_OBJECTS=$(GAME_OBJECTS) $(OBJECTS) $(FONT_OBJECTS)

default: ../lib/libgenie.a ../lib/libgeniegame.a

fnt_default.o: fnt_default.png
	ld -r -b binary -o fnt_default.o fnt_default.png
fnt_default_bold.o: fnt_default_bold.png
	ld -r -b binary -o fnt_default_bold.o fnt_default_bold.png
fnt_button.o: fnt_button.png
	ld -r -b binary -o fnt_button.o fnt_button.png
fnt_large.o: fnt_large.png
	ld -r -b binary -o fnt_large.o fnt_large.png

../lib:
	mkdir -p ../lib
../lib/libgenie.a: ../lib $(OBJECTS)
	ar -cr ../lib/libgenie.a $(OBJECTS)
../lib/libgeniegame.a: ../lib $(ALL_OBJECTS)
	ar -cr ../lib/libgeniegame.a $(ALL_OBJECTS)

clean:
	rm -f ../lib/libgenie.a ../lib/libgeniegame.a *.o
