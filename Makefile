CC=gcc
CFLAGS=-std=c99 -pedantic -Wall -Werror -W -rdynamic #-g #-DDEBUG
LDFLAGS=-ldl
SOURCES=$(wildcard src/*.c)
PLUGINS=$(wildcard plugins/*/*/*.c)
OBJECTS=$(SOURCES:.c=.o)
SO=$(PLUGINS:.c=.so)
EXECUTABLE=shelldone
.PHONY: clean clean-all debug plugins all prod

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(CFLAGS) $(DEBUG) $(LDFLAGS) $(OBJECTS) -o $@

%.o:
	$(CC) $(CFLAGS) $(DEBUG) -c -o $@ $(@:.o=.c)

debug: DEBUG = -g -DDEBUG

debug: clean $(EXECUTABLE)

prod: clean $(EXECUTABLE)

plugins: $(SO)
	@echo "Building plugins"

%.so:
	@echo "Building $@"
	@cd $(dir $@) && $(MAKE)

all: plugins $(EXECUTABLE)
	@echo "Building shelldone"

clean-all:
	@$(foreach obj, $(SO), eval "test -e $(obj) && rm $(obj) 2>/dev/null || true")
	@echo "Cleaning up all things"

clean:
	@$(foreach obj, $(OBJECTS), eval "test -e $(obj) && rm $(obj) 2>/dev/null || true")
	@test -e $(EXECUTABLE) && rm $(EXECUTABLE) 2>/dev/null || true
	@echo "Cleaning up things"

