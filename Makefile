CC=gcc
CFLAGS=-Wall -Werror -std=c89 -g -DNDEBUG -lreadline
LDFLAGS=
SOURCES=shelldone.c xutils.c command.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=shelldone
.PHONY: clean

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

clean:
	rm $(OBJECTS) $(EXECUTABLE)

