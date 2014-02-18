
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
CFLAGS  = -std=c99 -Ideps -Isrc
CFLAGS += -Wall -Wno-unused-function
LDFLAGS = -lcurl

example: example.c $(SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f example

.PHONY: clean
