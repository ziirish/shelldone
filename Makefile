CC=gcc
CFLAGS=-std=c99 -Wall -Werror -W -fno-builtin -g
LDFLAGS=
SOURCES=shelldone.c xutils.c command.c builtin.c parser.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=shelldone
.PHONY: clean

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)

