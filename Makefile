CC=gcc
CFLAGS=-Wall -Wextra -g

SRC=$(wildcard src/*.c)

# switch to bin/lan-server later
build:
	$(CC) $(CFLAGS) $(SRC) -o lan-server 
server:
	chmod +x lan-server
	./lan-server