.PHONY: default clean

WINDRES=i686-w64-mingw32-windres
OBJCOPY=i686-w64-mingw32-objcopy

CC=i686-w64-mingw32-gcc
CFLAGS=-Wall -Wextra -pedantic -std=gnu99 -g -DDEBUG

CXX=i686-w64-mingw32-g++
CXXFLAGS=-Wall -Wextra -pedantic -std=c++11 -g -DDEBUG

LDLIBS=-lstdc++ -lmingw32 -lgdi32 -lole32 -lcomdlg32 -lntdll -lwinmm

MAIN= \
	aoe \
	font \
	menu \
	regedit \
	rp \
	menu \
	setup \
	win

default: $(MAIN)

setup: setup.o setup.res

setup.res: setup.rc setupinfo.bin
	$(WINDRES) setup.rc -O coff -o setup.res

setupinfo.bin: setupinfo.o
	$(OBJCOPY) -O binary setupinfo.o setupinfo.bin

setup.o: setup.c cpu.h
	$(CC) -c ./setup.c -o $@ $(CFLAGS)

clean:
	rm -f $(MAIN) *.exe *.o *.res *.bin
