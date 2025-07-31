CC=g++
CXXFLAGS= -Wall -I/usr/include/dbus-1.0 -I/usr/lib64/dbus-1.0/include
LDFLAGS= -ldbus-1 -lncurses


all: debug main clean

debug: CXXFLAGS += -g -D DEBUG

main: bluelight.o main.o

.PHONY: clean debug

clean:
	rm *.o
