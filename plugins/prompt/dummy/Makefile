CC=gcc
CFLAGS=-std=c89 -Wall -Werror -W -g
LDFLAGS=
INCLUDES=-I../../../src
MOD_CFLAGS=-fPIC
MOD_LDFLAGS=-shared
SOURCES=$(wildcard *.c)
EXECUTABLE=dummy.so
.PHONY: clean

$(EXECUTABLE): 
	$(CC) $(MOD_LDFLAGS) $(MOD_CFLAGS) $(SOURCES) $(CFLAGS) -o $@ $(INCLUDES)

clean:
	rm $(EXECUTABLE)

