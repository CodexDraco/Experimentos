.PHONY: clean

CC = clang

RaylibPong: main.o
	$(CC) main.o -L../raylib/src -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -latomic -o RaylibPong

main.o: main.c
	$(CC) -I../raylib/src  -c main.c -o main.o

clean:
	rm -f main.o RaylibPong

