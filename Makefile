CC = gcc

INCLUDE = -Iinclude -I/home/eaglclaws/src/fuse-3.11.0/include
LIB = -L/usr/local/lib -L/home/eaglclaws/src/fuse-3.11.0/lib
LINK = -lcrypto -lfuse3 -lpthread -lsqlite3
FLAGS = -Wall -g

all: main.o sha256.o
	$(CC) $(FLAGS) $(FUSE) $(LIB) $(LINK) main.o sha256.o

main.o: src/main.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) $(FUSE) -c src/main.c -o main.o

sha256.o: src/sha256.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) -c src/sha256.c -o sha256.o

clean:
	rm *.o
