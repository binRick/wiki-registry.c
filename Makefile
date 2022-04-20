
CC     ?= cc
SRC     = $(wildcard src/*.c)
SRC    += $(wildcard deps/*/*.c)
CFLAGS  = -std=gnu99 -Ideps -Isrc
#CFLAGS += -Wno-unused-function
CFLAGS += -I include -I src -I . -I ../
LDFLAGS = -lcurl -lm -lparson
BIN = ./bin

test1: tests/test1.c $(SRC)
	@mkdir -p $(BIN)
	$(CC) $^ -o $(BIN)/$@ $(CFLAGS) $(LDFLAGS)

test-test1:
	@$(BIN)/test1 stars
	@$(BIN)/test1 fetch
	@$(BIN)/test1 encode

clean:
	rm -rf $(BIN)
	mkdir -p $(BIN)

dev: test1 tests

tests: test-test1

tidy:
	@./tidy.sh

.PHONY: clean
