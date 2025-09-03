CC=g++
CFLAGS=-fPIC -shared
MYSQL_CFLAGS=$(shell mysql_config --cflags)
MYSQL_LIBS=$(shell mysql_config --libs)

all: library.so main

library.so: library.cpp
	$(CC) $(CFLAGS) $(MYSQL_CFLAGS) -o library.so library.cpp $(MYSQL_LIBS)

main: main.go library.so
	go build -o main main.go

clean:
	rm -f library.so main

.PHONY: all clean