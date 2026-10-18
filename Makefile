CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g
SRC = src/lexer.c src/value.c src/ast.c src/parser.c src/interpreter.c src/diagnostics.c src/main.c
BIN = bin/neplang

.PHONY: all debug test clean

all: $(BIN)

$(BIN): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(BIN) $(SRC)

debug:
	mkdir -p bin
	$(CC) $(CFLAGS) -fsanitize=address,undefined -o $(BIN) $(SRC)

test: all
	bash tests/run_tests.sh

clean:
	rm -rf bin
