CC=gcc
CFLAGS=-std=c89 -Wall -Werror -W
LDFLAGS=
INCLUDES=-I../../../src
MOD_CFLAGS=-fPIC
MOD_LDFLAGS=-shared
SOURCES=$(wildcard *.c)
EXECUTABLE=jobs.so
.PHONY: clean

$(EXECUTABLE): 
	$(CC) $(MOD_LDFLAGS) $(MOD_CFLAGS) $(SOURCES) $(CFLAGS) -o $@ $(INCLUDES)

clean:
	rm $(EXECUTABLE)

