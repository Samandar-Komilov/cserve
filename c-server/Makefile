CC=gcc
CFLAGS=-Wall -Werror

all: build/server

build/server: build/Server.o build/test.o
	$(CC) $(CFLAGS) build/Server.o build/test.o -o build/server

build/Server.o: Server.c Server.h
	$(CC) $(CFLAGS) -c Server.c -o build/Server.o

build/test.o: test.c Server.h
	$(CC) $(CFLAGS) -c test.c -o build/test.o

clean:
	rm -f build/*.o build/server