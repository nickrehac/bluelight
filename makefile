DBUS_INCLUDE_DIR=$(shell pkg-config --cflags dbus-1)

CC=g++
CXXFLAGS= -Wall -Wno-sign-compare $(DBUS_INCLUDE_DIR)
LDFLAGS= -ldbus-1 -lncurses -lssl -lcrypto


all: clean debug main

release: clean main

release: CXXFLAGS += -O3

debug: CXXFLAGS += -g -D DEBUG

main:
	$(CC) $(CXXFLAGS) -o main main.cpp bluelight.cpp $(LDFLAGS)

.PHONY: clean debug release

clean:
	rm -f *.o
	rm -f main
