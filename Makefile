CC_LINUX = gcc
CC_MINGW = x86_64-w64-mingw32-gcc

CFLAGS = -Wall -Werror -g3

.PHONY: all clean build/m/cook.exe build/cook

all: build/cook

clean:
	rm -rf build

build/m/cook.exe: src/cook.c src/file.c src/lexer.c src/token.c
	mkdir -p build/m
	$(CC_MINGW) $(CFLAGS) src/file.c       -c -o build/m/file.o
	$(CC_MINGW) $(CFLAGS) src/token.c      -c -o build/m/token.o
	$(CC_MINGW) $(CFLAGS) src/lexer.c      -c -o build/m/lexer.o
	$(CC_MINGW) $(CFLAGS) src/arena.c      -c -o build/m/arena.o
	$(CC_MINGW) $(CFLAGS) src/parser.c     -c -o build/m/parser.o
	$(CC_MINGW) $(CFLAGS) src/expression.c -c -o build/m/expression.o
	$(CC_MINGW) $(CFLAGS) src/statement.c  -c -o build/m/statement.o
	$(CC_MINGW) $(CFLAGS) src/cook.c build/m/file.o build/m/token.o build/m/lexer.o build/m/arena.o build/m/parser.o build/m/expression.o build/m/statement.o -o build/m/cook.exe

build/cook: src/cook.c src/file.c src/lexer.c src/token.c
	mkdir -p build
	$(CC_LINUX) $(CFLAGS) src/file.c       -c -o build/file.o
	$(CC_LINUX) $(CFLAGS) src/token.c      -c -o build/token.o
	$(CC_LINUX) $(CFLAGS) src/lexer.c      -c -o build/lexer.o
	$(CC_LINUX) $(CFLAGS) src/arena.c      -c -o build/arena.o
	$(CC_LINUX) $(CFLAGS) src/parser.c     -c -o build/parser.o
	$(CC_LINUX) $(CFLAGS) src/expression.c -c -o build/expression.o
	$(CC_LINUX) $(CFLAGS) src/statement.c  -c -o build/statement.o
	$(CC_LINUX) $(CFLAGS) src/cook.c build/file.o build/token.o build/lexer.o build/arena.o build/parser.o build/expression.o build/statement.o -o build/cook

