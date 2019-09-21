.PHONY: default clean

FLAGS=-Wall -Wextra -pedantic -Wno-parentheses -Wno-implicit-fallthrough

CC?=gcc
CFLAGS=$(FLAGS) -std=c99

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

UI_LDLIBS=$(shell pkg-config --cflags --libs sdl2 gl openal) -lSDL2_image -lSDL2_mixer -lSDL2_ttf -lSDL2_gfx $(shell pkg-config --cflags --libs openssl)

ifdef CROSS_COMPILE
	CC=x86_64-w64-mingw32-gcc
	CFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"

	CXX=x86_64-w64-mingw32-g++
	CXXFLAGS += -I"/usr/local/x86_64-w64-mingw32/include/"

	LDFLAGS += -L"/usr/local/lib" -L"/usr/local/x86_64-w64-mingw32/lib/"
	#UI_LDLIBS = -lmingw32 -lSDL2 -lSDL2main -lSDL2_image -mwin32 -lopengl32 -lstdc++ -lz
	UI_LDLIBS = -lmingw32 -lSDL2main -lSDL2 -lSDL2_image
	LDLIBS=$(UI_LDLIBS)
endif

default: font
clean:
	rm -f font *.exe *.o
