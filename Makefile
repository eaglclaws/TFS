CC = gcc

INCLUDE = -Iinclude -I/home/eaglclaws/src/fuse-3.11.0/include
LIB = -L/usr/local/lib -L/home/eaglclaws/src/fuse-3.11.0/lib
MAIN_L = -lcrypto -lfuse3 -lpthread -lsqlite3
TOOL_L = -lsqlite3
FLAGS = -Wall -g -O2

all: tfs dbtool
	cp bin/* example

tfs: obj/main.o obj/sha256.o
	$(CC) $(FLAGS) $(LIB) $(MAIN_L) obj/main.o obj/sha256.o -o bin/tfs

dbtool: obj/dbtool.o
	$(CC) $(FLAGS) $(LIB) $(TOOL_L) obj/dbtool.o -o bin/dbtool


obj/main.o: src/main.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) $(FUSE) -c src/main.c -o obj/main.o

obj/sha256.o: src/sha256.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) -c src/sha256.c -o obj/sha256.o

obj/dbtool.o: src/dbtool.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/dbtool.c -o obj/dbtool.o

clean:
	rm obj/*.o
	rm bin/*
