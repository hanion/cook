CC_LINUX = gcc
CC_MINGW = x86_64-w64-mingw32-gcc
CFLAGS   = -Wall -Werror -Wpedantic -g3

SRCS := src/file.c src/token.c src/lexer.c src/arena.c src/parser.c src/expression.c src/statement.c src/interpreter.c src/symbol.c src/build_command.c src/cook.c
OBJS := $(SRCS:src/%.c=build/%.o)

MINGW_OBJS := $(SRCS:src/%.c=build/m/%.o)

LINUX_BIN := build/cook
MINGW_BIN := build/m/cook.exe

.PHONY: all clean run

all: $(MINGW_BIN) $(LINUX_BIN) run


$(LINUX_BIN): $(OBJS) | build
	$(CC_LINUX) $(CFLAGS) $^ -o $@

build/%.o: src/%.c | build
	$(CC_LINUX) $(CFLAGS) -c $< -o $@


$(MINGW_BIN): $(MINGW_OBJS)
	$(CC_MINGW) $(CFLAGS) $^ -o $@

build/m/%.o: src/%.c | build/m
	$(CC_MINGW) $(CFLAGS) -c $< -o $@


build/m:
	mkdir -p build/m

build:
	mkdir -p build

clean:
	rm -rf build

run: $(LINUX_BIN)
	./build/cook

