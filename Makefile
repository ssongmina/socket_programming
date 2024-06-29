
CC = gcc
CFLAGS = -Wall -Wextra -g

TARGET = myserver

SOURCES = servers.c

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f myserver
