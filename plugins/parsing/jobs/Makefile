CC=gcc
CFLAGS=-std=c89 -Wall -Werror -W #-g
LDFLAGS=
INCLUDES=-I../../../src
MOD_CFLAGS=-fPIC
MOD_LDFLAGS=-shared
SOURCES=$(wildcard *.c)
EXECUTABLE=jobs.so
.PHONY: clean

$(EXECUTABLE): clean
	$(CC) $(MOD_LDFLAGS) $(MOD_CFLAGS) $(SOURCES) $(CFLAGS) -o $@ $(INCLUDES)

clean:
	@(test -f $(EXECUTABLE) && rm $(EXECUTABLE) || echo "Already cleared")

