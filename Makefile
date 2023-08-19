CC=cc
CFLAGS=-Wall -g -Wextra -Wextra -std=c99 -pedantic -Wno-deprecated-declarations -Os
LIBS=""
BIN=todo
SRC=main.c

all: main.c
	$(CC) $(SRC) -o $(BIN) $(CFLAGS)
