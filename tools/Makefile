.PHONY: clean

CC?=gcc
CXX?=g++
CFLAGS?=-Wall -Wextra -pedantic -std=gnu11 -g -DDEBUG
CXXFLAGS?=-Wall -Wextra -pedantic -std=c++11 -g -DDEBUG
LDLIBS=-lz -lstdc++

../empires/inflato.o: ../empires/inflate.cpp ../empires/serialization.h
scn.o: scn.c ../empires/serialization.h
scn: scn.o inflate.o

clean:
	rm -f convert pestat wave drs scn dview *.o
