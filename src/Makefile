CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -W -rdynamic #-DDEBUG
LDFLAGS=-ldl
SOURCES=$(wildcard *.c)
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=shelldone
.PHONY: clean

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)

