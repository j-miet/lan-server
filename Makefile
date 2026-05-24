CC=gcc
CFLAGS=-Wall -Wextra -g

SRC=$(wildcard src/*.c src/**/*.c)
OUT=bin/lan-server

build:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)
server:
	$(OUT)