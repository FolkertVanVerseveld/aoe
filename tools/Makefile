.PHONY: default clean

CC?=gcc
CXX?=g++
CFLAGS?=-Wall -Wextra -pedantic -std=gnu11 -g -DDEBUG
CXXFLAGS?=-Wall -Wextra -pedantic -std=c++11 -g -DDEBUG
LDLIBS=-lz -lstdc++

default: cheat convert pestat wave drs scn rsrc

../empires/inflate.o: ../empires/inflate.cpp ../empires/serialization.h
scn.o: scn.c ../empires/serialization.h
cheat: cheat.c
scn: scn.o ../empires/inflate.o
	$(CC) $(CFLAGS) -o scn scn.o ../empires/inflate.o $(LDLIBS)
wave: wave.c
	$(CC) $(CFLAGS) -o wave wave.c $(LDLIBS) $(shell pkg-config --libs openal)
rsrc: rsrc.o xmap.o
	$(CC) $(CFLAGS) -o rsrc rsrc.o xmap.o

clean:
	rm -f cheat convert pestat wave drs scn rsrc dview *.o
