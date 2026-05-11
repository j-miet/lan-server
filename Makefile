CC=gcc
CFLAGS=-Wall -Wextra -g

SRC=$(wildcard src/*.c)

build:
	$(CC) $(CFLAGS) $(SRC) -o lan-server
server:
	chmod +x lan-server
	./lan-server
test:
	$(CC) $(CFLAGS) $(SRC) -o lan-server
	chmod +x lan-server
	./lan-server
