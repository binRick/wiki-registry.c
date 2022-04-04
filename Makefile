
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
CFLAGS  = -std=gnu99 -Ideps -Isrc
CFLAGS += -Wno-unused-function
CFLAGS += -I include -I src -I . -I ../
LDFLAGS = -lcurl -lm

gh: gh.c $(SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

#stars: stars.c $(SRC)
#	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

example: example.c $(SRC)
	$(CC) $^ -o $@ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f example gh stars

dev: example

test:
	@./example 

.PHONY: clean
