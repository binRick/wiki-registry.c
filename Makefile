
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
CFLAGS  = -std=gnu99 -Ideps -Isrc
CFLAGS += -Wall -Wno-unused-function
CFLAGS += -I ../.
LDFLAGS = -lcurl -lm

gh: gh.c $(SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

example: example.c $(SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f example gh

.PHONY: clean
