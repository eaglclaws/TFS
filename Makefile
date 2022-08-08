CC = gcc

INCLUDE = -Iinclude
LIB = -L/usr/local/lib
LINK = -lcrypto
FLAGS = -Wall
FUSE = `pkg-config fuse3 --cflags --libs`

all: main.o sha256.o
	$(CC) $(FLAGS) $(FUSE) $(LIB) $(LINK) main.o sha256.o

main.o: src/main.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) $(FUSE) -c src/main.c -o main.o

sha256.o: src/sha256.c include/sha256.h
	$(CC) $(FLAGS) $(INCLUDE) -c src/sha256.c -o sha256.o

clean:
	rm *.o
