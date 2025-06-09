CC_LINUX = gcc
CC_MINGW = x86_64-w64-mingw32-gcc
CFLAGS   = -Wall -Werror -g3

SRCS := src/file.c src/token.c src/lexer.c src/arena.c src/parser.c src/expression.c src/statement.c src/cook.c
OBJS := $(SRCS:src/%.c=build/%.o)

MINGW_OBJS := $(SRCS:src/%.c=build/m/%.o)

LINUX_BIN := build/cook
MINGW_BIN := build/m/cook.exe

.PHONY: all clean $(LINUX_BIN)

all: $(MINGW_BIN) $(LINUX_BIN) run


$(LINUX_BIN): $(OBJS)
	$(CC_LINUX) $(CFLAGS) $^ -o $@

build/%.o: src/%.c
	$(CC_LINUX) $(CFLAGS) -c $< -o $@


$(MINGW_BIN): $(MINGW_OBJS)
	$(CC_MINGW) $(CFLAGS) $^ -o $@

build/m/%.o: src/%.c
	$(CC_MINGW) $(CFLAGS) -c $< -o $@


build/m:
	mkdir -p $(dir $@)

build:
	mkdir -p $(dir $@)

clean:
	rm -rf build

run:
	./build/cook

