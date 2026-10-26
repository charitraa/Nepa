CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -g
SRC = src/lexer.c src/value.c src/ast.c src/parser.c src/interpreter.c src/diagnostics.c src/main.c
BIN = bin/neplang
PREFIX ?= /usr/local

.PHONY: all debug test clean install uninstall

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

install: $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 755 $(BIN) $(DESTDIR)$(PREFIX)/bin/neplang
	install -m 644 docs/neplang.1 $(DESTDIR)$(PREFIX)/share/man/man1/neplang.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/neplang $(DESTDIR)$(PREFIX)/share/man/man1/neplang.1
