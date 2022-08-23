CC = gcc

INCLUDE = -Iinclude -I/home/eaglclaws/src/fuse-3.11.0/include
LIB = -L/usr/local/lib -L/home/eaglclaws/src/fuse-3.11.0/lib
LINK = -lcrypto -lfuse3 -lpthread -lsqlite3
FLAGS = -Wall -g

all: tfs dbtool
	cp bin/* example

tfs: main.o sha256.o
	$(CC) $(FLAGS) $(FUSE) $(LIB) $(LINK) obj/main.o obj/sha256.o -o bin/tfs

dbtool: dbtool.o
	$(CC) $(FLAGS) $(LIB) $(LINK) obj/dbtool.o -o bin/dbtool


main.o: src/main.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) $(FUSE) -c src/main.c -o obj/main.o

sha256.o: src/sha256.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) -c src/sha256.c -o obj/sha256.o

dbtool.o: src/dbtool.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/dbtool.c -o obj/dbtool.o

clean:
	rm obj/*.o
	rm bin/*
