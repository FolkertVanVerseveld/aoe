.PHONY: default clean

FLAGS=-Wall -Wextra -pedantic -Wno-parentheses -Wno-implicit-fallthrough -I../include/ $(shell pkg-config --cflags xtcommon)

FONT_OBJECTS=\
	../genie/fnt_data.o \
	../genie/fnt_default.o \
	../genie/fnt_default_bold.o \
	../genie/fnt_button.o \
	../genie/fnt_large.o

CC?=gcc
CFLAGS=$(FLAGS) -std=gnu11
LDLIBS=-lm -lz
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

LDLIBS += $(shell pkg-config --cflags --libs sdl2 gl openal) -lSDL2_image -lSDL2_mixer $(shell pkg-config --cflags --libs openssl)

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

	LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_mixer -mwin32 -lopengl32 -lstdc++ -lz -lcomdlg32

	CFLAGS += -I"/usr/local/include/"
	CXXFLAGS += -I"/usr/local/include/"
endif

LDLIBS += $(shell pkg-config --libs xtcommon)

default: empiresx

empiresx: empiresx.o $(FONT_OBJECTS)
empiresx.o: empiresx.c

clean:
	rm -f empiresx *.o
