CC=gcc
CFLAGS=-Wall -Wextra -g

SRC=$(wildcard src/*.c)
OUT=bin/lan-server

# switch to bin/lan-server later
build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)
server:
	$(OUT)