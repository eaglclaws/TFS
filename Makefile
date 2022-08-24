CC = gcc

INCLUDE = -Iinclude -I/home/eaglclaws/src/fuse-3.11.0/include
LIB = -L/usr/local/lib -L/home/eaglclaws/src/fuse-3.11.0/lib
MAIN_L = -lcrypto -lfuse3 -lpthread -lsqlite3
TOOL_L = -lsqlite3
FLAGS = -Wall -g -O2

all: tfs dbtool mktag rmtag tag
	cp bin/* example

tfs: obj/main.o obj/sha256.o
	$(CC) $(FLAGS) $(LIB) $(MAIN_L) obj/main.o obj/sha256.o -o bin/tfs

dbtool: obj/dbtool.o
	$(CC) $(FLAGS) $(LIB) $(TOOL_L) obj/dbtool.o -o bin/dbtool

rmtag: obj/rmtag.o
	$(CC) $(FLAGS) $(LIB) $(TOOL_L) obj/rmtag.o -o bin/rmtag

mktag: obj/mktag.o
	$(CC) $(FLAGS) $(LIB) $(TOOL_L) obj/mktag.o -o bin/mktag

tag: obj/tag.o
	$(CC) $(FLAGS) $(LIB) $(TOOL_L) obj/tag.o -o bin/tag

obj/main.o: src/main.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) $(FUSE) -c src/main.c -o obj/main.o

obj/sha256.o: src/sha256.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) -c src/sha256.c -o obj/sha256.o

obj/dbtool.o: src/dbtool.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/dbtool.c -o obj/dbtool.o
  
obj/rmtag.o: src/rmtag.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/rmtag.c -o obj/rmtag.o

obj/mktag.o: src/mktag.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/mktag.c -o obj/mktag.o

obj/tag.o: src/tag.c
	$(CC) $(FLAGS) $(INCLUDE) -c src/tag.c -o obj/tag.o

clean:
	rm obj/*.o
	rm bin/*
