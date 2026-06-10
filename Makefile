.PHONY: all build tests test serve clean

CC ?= gcc
CPPFLAGS += -I./src -D_FILE_OFFSET_BITS=64
CFLAGS += -Wall -Wextra -Werror -std=gnu2x -g

SERVER_SRC := $(shell find src -name '*.c')
TEST_SRC := $(filter-out src/main.c,$(SERVER_SRC))

TESTS := $(shell find tests -name '*.c')

OUT := bin/lan-server
TEST_OUT := bin/tests

all: build

tests:
	mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRC) $(TESTS) -o $(TEST_OUT)

test: tests
	./$(TEST_OUT)

build:
	mkdir -p bin
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SERVER_SRC) -o $(OUT)

server: build
	./$(OUT)

clean:
	rm -f $(OUT) $(TEST_OUT)