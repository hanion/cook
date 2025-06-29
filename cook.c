/*
    cook - better make - haion.dev - https://github.com/hanion/cook

    a single file build system.
    build `cook.c` once, run it, and it builds your project via a `Cookfile`.
    requires only a C compiler, no external tools needed.

    you can ship it with your project.
    just add `cook.c` to your project, and write a `Cookfile`.


    usage:
        $ cc -o cook cook.c
        $ ./cook

    includes code from:
        nob - v1.20.6 - Public Domain - https://github.com/tsoding/nob.h


    minimal:
            build(main)
        runs:
            cc -o main main.c


    chaining and nesting:
            build(foo).build(bar)
        is the same as:
            build(foo) {
                build(bar)
            }
        runs:
            cc -c -o bar.o bar.c
            cc -o foo foo.c bar.o


    context inheritance:
        when you write a build(), it inherits the parent build command's settings.
        example:
            build(foo).cflags(-Wall, -Wextra)
            build(bar).cflags(-Wall, -Wextra)
        is the same as:
            cflags(-Wall, -Wextra)
            build(foo)
            build(bar)
        both 'foo' and 'bar' are built with the same flags.
        they inherit flags from the current context at the time they're defined.


    typical Cookfile:
        compiler(gcc)
        cflags(-Wall, -Werror, -Wpedantic, -g3)
        source_dir(src)
        output_dir(build)

        build(tester).build(file)

        build(cook) {
            build(file, token, lexer, arena, parser, expression,
                  statement, interpreter, symbol, build_command, main)
        }
    how it works:
        - all builds inherit settings from the surrounding context (compiler, flags, dirs)
        - build(tester).build(file) compiles file.c first, then tester.c with file.o
        - build(cook) { ... } declares a nested build:
            - this produces the final executable: build/cook
            - inside, object dependencies are listed (compiled before linking)
        - all source paths are relative to source_dir
        - input files can be given without extensions, .c or .cpp is inferred
        - no need to manually track .o files


    complex build:
        compiler(g++)
        cflags(-Wall, -Wextra, -Werror, -g)
        source_dir(src)
        output_dir(build)
        include_dir(include, imgui, libs/glfw/include, libs/glew/include, libs/glm)
        library_dir(libs/glfw/lib, libs/glew/lib)

        build(game) {
            link(glfw, GLEW, GL, dl, pthread)
            build(main, input, window, renderer, imgui)
        }
    explanation:
        - build(game) defines the final output: build/game
        - nested builds are object files compiled before linking
        - inherits all settings above (compiler, flags, dirs)
*/



#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define da_reserve(da, expected_capacity)                                                  \
	do {                                                                                   \
		if ((expected_capacity) > (da)->capacity) {                                        \
			if ((da)->capacity == 0) {                                                     \
				(da)->capacity = 256;                                                      \
			}                                                                              \
			while ((expected_capacity) > (da)->capacity) {                                 \
				(da)->capacity *= 2;                                                       \
			}                                                                              \
			(da)->items = realloc((da)->items, (da)->capacity * sizeof(*(da)->items));     \
			assert((da)->items != NULL);                                                   \
		}                                                                                  \
	} while (0)

#define da_append(da, item)                    \
	do {                                       \
		da_reserve((da), (da)->count + 1);     \
		(da)->items[(da)->count++] = (item);   \
	} while (0)

#define da_append_many(da, new_items, new_items_count)                                          \
	do {                                                                                        \
		da_reserve((da), (da)->count + (new_items_count));                                      \
		memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
		(da)->count += (new_items_count);                                                       \
	} while (0)


#define da_reserve_arena(arena, da, expected_capacity)                                     \
	do {                                                                                   \
		size_t old_size = (da)->capacity;                                                  \
		if ((expected_capacity) > (da)->capacity) {                                        \
			if ((da)->capacity == 0) {                                                     \
				(da)->capacity = 256;                                                      \
			}                                                                              \
			while ((expected_capacity) > (da)->capacity) {                                 \
				(da)->capacity *= 2;                                                       \
			}                                                                              \
			(da)->items = arena_realloc((arena), (da)->items,                              \
				old_size, (da)->capacity * sizeof(*(da)->items));                          \
			assert((da)->items != NULL);                                                   \
		}                                                                                  \
	} while (0)

#define da_append_arena(arena, da, item)                  \
	do {                                                  \
		da_reserve_arena((arena), (da), (da)->count + 1); \
		(da)->items[(da)->count++] = (item);              \
	} while (0)

#define da_append_many_arena(arena, da, new_items, new_items_count)                             \
	do {                                                                                        \
		da_reserve_arena((arena), (da), (da)->count + (new_items_count));                       \
		memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
		(da)->count += (new_items_count);                                                       \
	} while (0)


typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} StringBuilder;

static inline void sb_free(StringBuilder* sb) {
	sb->count = 0;
	sb->capacity = 0;
	free(sb->items);
}

typedef struct {
	const char* items;
	size_t count;
} StringView;


typedef struct StringList {
	StringView* items;
	size_t count;
	size_t capacity;
} StringList;




static inline StringView sv_from_sb(StringBuilder sb) {
	return (StringView) {
		.count = sb.count,
		.items = sb.items
	};
}
static inline StringBuilder sb_from_sv(StringView sv) {
	return (StringBuilder) {
		.count = sv.count,
		.items = (char*)sv.items,
		.capacity = sv.count
	};
}

#include <stdbool.h>
#include <sys/stat.h>

bool read_entire_file(const char *filepath_cstr, StringBuilder *sb);
bool write_to_file   (const char *filepath_cstr, StringBuilder *sb);

const char* get_filename(const char* filepath_cstr);

bool ends_with(const char* cstr, const char* w);


#ifdef _WIN32
	#define MKDIR(path) mkdir(path)
#else
	#include <unistd.h>
	#define MKDIR(path) mkdir(path, 0777)
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


bool read_entire_file(const char *filepath_cstr, StringBuilder *sb) {
	bool result = true;

	FILE *f = fopen(filepath_cstr, "rb");
	if (f == NULL) { result = false; goto defer; }
	if (fseek(f, 0, SEEK_END) < 0) { result = false; goto defer; }

#ifndef _WIN32
	long m = ftell(f);
#else
	long long m = _ftelli64(f);
#endif

	if (m < 0) { result = false; goto defer; }
	if (fseek(f, 0, SEEK_SET) < 0) { result = false; goto defer; }

	size_t new_count = sb->count + m;
	if (new_count > sb->capacity) {
		sb->items = realloc(sb->items, new_count);
		assert(sb->items != NULL);
		sb->capacity = new_count;
	}

	fread(sb->items + sb->count, m, 1, f);
	if (ferror(f)) { result = false; goto defer; }
	sb->count = new_count;

defer:
	if (!result) { printf("Could not read file %s: %s\n", filepath_cstr, strerror(errno)); }
	if (f) { fclose(f); }
	return result;
}

bool write_to_file(const char *filepath_cstr, StringBuilder *sb) {
	FILE *f = fopen(filepath_cstr, "wb");
	if (f == NULL) {
		printf("Could not open file for writing: %s\n", strerror(errno));
		return false;
	}

	size_t written = fwrite(sb->items, 1, sb->count, f);
	if (written != sb->count) {
		printf("Error writing to file: %s\n", strerror(errno));
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}


const char* get_filename(const char* filepath_cstr) {
	const char* last_slash = strrchr(filepath_cstr, '/');
	if (!last_slash) {
		return filepath_cstr;
	}
	return last_slash + 1;
}

bool ends_with(const char* cstr, const char* w) {
	size_t len_str = strlen(cstr);
	size_t len_w = strlen(w);
	if (len_w > len_str) {
		return false;
	}
	return strcmp(cstr + len_str - len_w, w) == 0;
}


#include <stddef.h>
#include <stdbool.h>

typedef enum {
	TOKEN_END = 0,
	TOKEN_INVALID,

	TOKEN_IDENTIFIER,
	TOKEN_STRING_LITERAL, TOKEN_INTEGER_LITERAL, TOKEN_FLOAT_LITERAL,

	TOKEN_OPEN_PAREN, TOKEN_CLOSE_PAREN,
	TOKEN_OPEN_CURLY, TOKEN_CLOSE_CURLY,
	TOKEN_OPEN_BRACKET, TOKEN_CLOSE_BRACKET,

	TOKEN_COMMA, TOKEN_DOT,
	TOKEN_SEMICOLON, TOKEN_COLON,
	TOKEN_QUESTION,

	TOKEN_AT, TOKEN_DOLLAR,

	TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
	TOKEN_PERCENT, TOKEN_AMPERSAND,
	TOKEN_PIPE, TOKEN_CARET,
	TOKEN_TILDE, TOKEN_EXCLAMATION,

	TOKEN_EQUAL, TOKEN_PLUS_EQUAL, TOKEN_MINUS_EQUAL,
	TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_PERCENT_EQUAL,
	TOKEN_AMPERSAND_EQUAL, TOKEN_PIPE_EQUAL, TOKEN_CARET_EQUAL,

	TOKEN_EQUAL_EQUAL, TOKEN_EXCLAMATION_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,

	TOKEN_PLUS_PLUS, TOKEN_MINUS_MINUS,
	TOKEN_SHIFT_LEFT, TOKEN_SHIFT_RIGHT,

	//TOKEN_SHIFT_LEFT_EQUAL, TOKEN_SHIFT_RIGHT_EQUAL,

	TOKEN_AND_AND, TOKEN_OR_OR,

	TOKEN_KEYWORD_IF,
	TOKEN_KEYWORD_ELSE,
	TOKEN_KEYWORD_FOR,
	TOKEN_KEYWORD_WHILE,
	TOKEN_KEYWORD_BREAK,
	TOKEN_KEYWORD_CONTINUE,
	TOKEN_KEYWORD_RETURN,
	TOKEN_KEYWORD_SWITCH,
	TOKEN_KEYWORD_CASE,
	TOKEN_KEYWORD_DEFAULT,
	TOKEN_KEYWORD_TRUE,
	TOKEN_KEYWORD_FALSE
} TokenType;


typedef struct {
	TokenType type;
	StringView str;
	size_t line;
	size_t column;
} Token;


const char* token_name_cstr(Token token);
const char* token_type_name_cstr(TokenType tt);
TokenType token_lookup_keyword(const Token token);

bool token_is_integer     (char c);
bool token_is_symbol_start(char c);
bool token_is_symbol      (char c);

bool token_equals(Token token, const char* cstr);

void token_print(Token t, int indent);

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char* token_name_cstr(Token token) {
	return token_type_name_cstr(token.type);
}


const char* token_type_name_cstr(TokenType tt) {
	switch (tt) {
		#define CASE(T) case T: return #T;
		case TOKEN_INVALID: return "!!! TOKEN_INVALID";
		CASE(TOKEN_END)
		CASE(TOKEN_IDENTIFIER)
		CASE(TOKEN_STRING_LITERAL) CASE(TOKEN_INTEGER_LITERAL) CASE(TOKEN_FLOAT_LITERAL)

		CASE(TOKEN_OPEN_PAREN) CASE(TOKEN_CLOSE_PAREN)
		CASE(TOKEN_OPEN_CURLY) CASE(TOKEN_CLOSE_CURLY)
		CASE(TOKEN_OPEN_BRACKET) CASE(TOKEN_CLOSE_BRACKET)
		CASE(TOKEN_COMMA) CASE(TOKEN_DOT)
		CASE(TOKEN_SEMICOLON) CASE(TOKEN_COLON)
		CASE(TOKEN_QUESTION)

		CASE(TOKEN_AT) CASE(TOKEN_DOLLAR)

		CASE(TOKEN_PLUS) CASE(TOKEN_MINUS) CASE(TOKEN_STAR) CASE(TOKEN_SLASH)
		CASE(TOKEN_PERCENT) CASE(TOKEN_AMPERSAND)
		CASE(TOKEN_PIPE) CASE(TOKEN_CARET)
		CASE(TOKEN_TILDE) CASE(TOKEN_EXCLAMATION)

		CASE(TOKEN_EQUAL) CASE(TOKEN_PLUS_EQUAL) CASE(TOKEN_MINUS_EQUAL)
		CASE(TOKEN_STAR_EQUAL) CASE(TOKEN_SLASH_EQUAL) CASE(TOKEN_PERCENT_EQUAL)
		CASE(TOKEN_AMPERSAND_EQUAL) CASE(TOKEN_PIPE_EQUAL) CASE(TOKEN_CARET_EQUAL)

		CASE(TOKEN_EQUAL_EQUAL) CASE(TOKEN_EXCLAMATION_EQUAL)
		CASE(TOKEN_LESS) CASE(TOKEN_LESS_EQUAL)
		CASE(TOKEN_GREATER) CASE(TOKEN_GREATER_EQUAL)

		CASE(TOKEN_PLUS_PLUS) CASE(TOKEN_MINUS_MINUS)
		CASE(TOKEN_SHIFT_LEFT) CASE(TOKEN_SHIFT_RIGHT)

		//CASE(TOKEN_SHIFT_LEFT_EQUAL) CASE(TOKEN_SHIFT_RIGHT_EQUAL)

		CASE(TOKEN_AND_AND) CASE(TOKEN_OR_OR)

		CASE(TOKEN_KEYWORD_IF)
		CASE(TOKEN_KEYWORD_ELSE)
		CASE(TOKEN_KEYWORD_FOR)
		CASE(TOKEN_KEYWORD_WHILE)
		CASE(TOKEN_KEYWORD_BREAK)
		CASE(TOKEN_KEYWORD_CONTINUE)
		CASE(TOKEN_KEYWORD_RETURN)
		CASE(TOKEN_KEYWORD_SWITCH)
		CASE(TOKEN_KEYWORD_CASE)
		CASE(TOKEN_KEYWORD_DEFAULT)
		CASE(TOKEN_KEYWORD_TRUE)
		CASE(TOKEN_KEYWORD_FALSE)
		#undef CASE
	}
	return "!!! TOKEN INVALID";
}

TokenType token_lookup_keyword(const Token token) {
	#define KW(k,l) (token.str.count == l && 0 == strncmp(k, token.str.items, l))
	if (KW("if", 2))       { return TOKEN_KEYWORD_IF; }
	if (KW("else", 4))     { return TOKEN_KEYWORD_ELSE; }
	if (KW("for", 3))      { return TOKEN_KEYWORD_FOR; }
	if (KW("while", 5))    { return TOKEN_KEYWORD_WHILE; }
	if (KW("break", 5))    { return TOKEN_KEYWORD_BREAK; }
	if (KW("continue", 8)) { return TOKEN_KEYWORD_CONTINUE; }
	if (KW("return", 6))   { return TOKEN_KEYWORD_RETURN; }
	if (KW("switch", 6))   { return TOKEN_KEYWORD_SWITCH; }
	if (KW("case", 4))     { return TOKEN_KEYWORD_CASE; }
	if (KW("default", 7))  { return TOKEN_KEYWORD_DEFAULT; }
	if (KW("true", 4))     { return TOKEN_KEYWORD_TRUE; }
	if (KW("false", 5))    { return TOKEN_KEYWORD_FALSE; }
	#undef KW
	return TOKEN_INVALID;
}

bool token_is_integer(char c) {
	return (c >= '0' && c <= '9');
}

bool token_is_symbol_start(char c) {
	return (unsigned char)c >= 128 || isalpha(c) || c == '_';
}

bool token_is_symbol(char c) {
	return (unsigned char)c >= 128 || isalnum(c) || c == '_';
}


bool token_equals(Token token, const char* cstr) {
	if(token.str.count != strlen(cstr)) {
		return false;
	}
	return memcmp(token.str.items, cstr, token.str.count) == 0;
}

void token_print(Token t, int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
	printf("token: %3zu:%-3zu %-18s '%.*s'\n",
		 t.line, t.column, token_name_cstr(t), (int)t.str.count, t.str.items);
}





typedef struct {
	const char* content;
	size_t content_length;
	size_t cursor;
	size_t line;
	size_t line_start;
} Lexer;

Lexer lexer_new(StringView sv);

char lexer_advance         (Lexer* l);
bool lexer_check           (Lexer* l, char c);
bool lexer_match           (Lexer* l, char c);
bool lexer_check_next      (Lexer* l, char c);
void lexer_trim_left       (Lexer* l);
void lexer_skip_to_new_line(Lexer* l);

Token lexer_next_token(Lexer* l);
Token lexer_peek_next (Lexer* l);

void lexer_dump(Lexer* l);

#include <assert.h>
#include <ctype.h>


Lexer lexer_new(StringView sv) {
	Lexer lexer = {
		.content = sv.items,
		.content_length = sv.count,
		.cursor = 0,
		.line = 0,
	};
	return lexer;
}

char lexer_advance(Lexer* l) {
	assert(l->cursor < l->content_length);
	char c = l->content[l->cursor];
	l->cursor++;
	return c;
}

bool lexer_check(Lexer* l, char c) {
	if (l->cursor < l->content_length) {
		return c == l->content[l->cursor];
	}
	return false;
}
bool lexer_match(Lexer* l, char c) {
	if (lexer_check(l, c)) {
		lexer_advance(l);
		return true;
	}
	return false;
}

bool lexer_check_next(Lexer* l, char c) {
	if (l->cursor + 1 < l->content_length) {
		return c == l->content[l->cursor + 1];
	}
	return false;
}

// does trim newline
void lexer_trim_left(Lexer* l) {
	while (l->cursor < l->content_length && isspace(l->content[l->cursor])) {
		if (l->content[l->cursor] == '\n') {
			l->line++;
			l->line_start = l->cursor;
		}
		lexer_advance(l);
	}
}
void lexer_skip_to_new_line(Lexer* l) {
	while (l->cursor < l->content_length) {
		lexer_advance(l);
		if (l->content[l->cursor] == '\n') {
			l->line++;
			l->line_start = l->cursor;
			break;
		}
	}
}

Token lexer_next_token(Lexer* l) {
	lexer_trim_left(l);

	Token token = {
		.type = TOKEN_INVALID,
		.str.items = &l->content[l->cursor],
		.str.count = 0,
		.line = l->line,
		.column = l->cursor - l->line_start,
	};

	if (l->cursor >= l->content_length) {
		token.type = TOKEN_END;
		return token;
	}

	// skip comments
	if (lexer_check(l, '#') || (lexer_check(l, '/') && lexer_check_next(l, '/'))) {
		lexer_skip_to_new_line(l);
		return lexer_next_token(l);
	}

	if (token_is_symbol_start(l->content[l->cursor])) {
		token.type = TOKEN_IDENTIFIER;
		while (l->cursor < l->content_length && token_is_symbol(l->content[l->cursor])) {
			l->cursor++;
			token.str.count++;
		}

		TokenType keyword = token_lookup_keyword(token);
		if (keyword != TOKEN_INVALID) {
			token.type = keyword;
		}

		return token;
	}

	if (token_is_integer(l->content[l->cursor])) {
		while (l->cursor < l->content_length && token_is_integer(l->content[l->cursor])) {
			l->cursor++;
			token.str.count++;
		}
		if (l->cursor < l->content_length && l->content[l->cursor] == '.') {
			l->cursor++;
			token.str.count++;
			while (l->cursor < l->content_length && token_is_integer(l->content[l->cursor])) {
				l->cursor++;
				token.str.count++;
			}
			token.type = TOKEN_FLOAT_LITERAL;
		} else {
			token.type = TOKEN_INTEGER_LITERAL;
		}
		return token;
	}

	if (lexer_match(l, '"')) {
		token.str.items++; // skip first "
		token.type = TOKEN_STRING_LITERAL;
		while (l->cursor + 1 < l->content_length) {
			l->cursor++;
			token.str.count++;
			if (lexer_match(l, '"')) { // skip last "
				break;
			}
		}
		return token;
	}

	switch (l->content[l->cursor]) {
		#define T_RETN(tt, n) { token.type = tt; token.str.count = n; l->cursor += n; return token; }
		#define T_RET(tt) T_RETN(tt,1)
		#define T_RET_IF_NEXT(c, tt)  if (lexer_check_next(l, c)) { T_RETN(tt,2); }

		case '(': T_RET(TOKEN_OPEN_PAREN);
		case ')': T_RET(TOKEN_CLOSE_PAREN);
		case '{': T_RET(TOKEN_OPEN_CURLY);
		case '}': T_RET(TOKEN_CLOSE_CURLY);
		case '[': T_RET(TOKEN_OPEN_BRACKET);
		case ']': T_RET(TOKEN_CLOSE_BRACKET);
		case ',': T_RET(TOKEN_COMMA);
		case '.': T_RET(TOKEN_DOT);
		case ';': T_RET(TOKEN_SEMICOLON);
		case ':': T_RET(TOKEN_COLON);
		case '?': T_RET(TOKEN_QUESTION);
		case '@': T_RET(TOKEN_AT);
		case '$': T_RET(TOKEN_DOLLAR);
		case '~': T_RET(TOKEN_TILDE);

		case '+':
			T_RET_IF_NEXT('+', TOKEN_PLUS_PLUS);
			T_RET_IF_NEXT('=', TOKEN_PLUS_EQUAL);
			T_RET(TOKEN_PLUS);

		case '-':
			T_RET_IF_NEXT('-', TOKEN_MINUS_MINUS);
			T_RET_IF_NEXT('=', TOKEN_MINUS_EQUAL);
			T_RET(TOKEN_MINUS);

		case '*':
			T_RET_IF_NEXT('=', TOKEN_STAR_EQUAL);
			T_RET(TOKEN_STAR);

		case '/':
			T_RET_IF_NEXT('=', TOKEN_SLASH_EQUAL);
			T_RET(TOKEN_SLASH);

		case '%':
			T_RET_IF_NEXT('=', TOKEN_PERCENT_EQUAL);
			T_RET(TOKEN_PERCENT);

		case '&':
			T_RET_IF_NEXT('&', TOKEN_AND_AND);
			T_RET_IF_NEXT('=', TOKEN_AMPERSAND_EQUAL);
			T_RET(TOKEN_AMPERSAND);

		case '|':
			T_RET_IF_NEXT('|', TOKEN_OR_OR);
			T_RET_IF_NEXT('=', TOKEN_PIPE_EQUAL);
			T_RET(TOKEN_PIPE);

		case '^':
			T_RET_IF_NEXT('=', TOKEN_CARET_EQUAL);
			T_RET(TOKEN_CARET);

		case '=':
			T_RET_IF_NEXT('=', TOKEN_EQUAL_EQUAL);
			T_RET(TOKEN_EQUAL);

		case '!':
			T_RET_IF_NEXT('=', TOKEN_EXCLAMATION_EQUAL);
			T_RET(TOKEN_EXCLAMATION);

		case '<':
			T_RET_IF_NEXT('<', TOKEN_SHIFT_LEFT);
			T_RET_IF_NEXT('=', TOKEN_LESS_EQUAL);
			T_RET(TOKEN_LESS);

		case '>':
			T_RET_IF_NEXT('<', TOKEN_SHIFT_RIGHT);
			T_RET_IF_NEXT('=', TOKEN_GREATER_EQUAL);
			T_RET(TOKEN_GREATER);

		#undef T_RET
		#undef T_RETN
		#undef T_RET_IF_NEXT
	}

	l->cursor++;
	token.str.count = 1;
	token.type = TOKEN_INVALID;
	return token;
}


Token lexer_peek_next(Lexer* l) {
	Lexer copy = *l;
	return lexer_next_token(&copy);
}


void lexer_dump(Lexer* l) {
	Lexer copy = *l;
	Token t = lexer_next_token(&copy);
	while (t.type != TOKEN_END) {
		token_print(t, 1);
		t = lexer_next_token(&copy);
	}
}


#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

typedef struct Region {
	struct Region *next;
	size_t capacity;
	size_t size;
	char buffer[];
} Region;

Region *region_new(size_t capacity);

#define ARENA_DEFAULT_CAPACITY (640 * 1000)

typedef struct {
    Region *first;
    Region *last;
} Arena;

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment);
void *arena_alloc(Arena *arena, size_t size);
void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size);
void arena_clean(Arena *arena);
void arena_free(Arena *arena);
void arena_summary(Arena *arena);


#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

Region *region_new(size_t capacity) {
	const size_t region_size = sizeof(Region) + capacity;
	Region *region = malloc(region_size);
	memset(region, 0, region_size);
	region->capacity = capacity;
	return region;
}

void *arena_alloc_aligned(Arena *arena, size_t size, size_t alignment) {
	if (arena->last == NULL) {
		assert(arena->first == NULL);

		Region *region = region_new(
			size > ARENA_DEFAULT_CAPACITY ? size : ARENA_DEFAULT_CAPACITY);

		arena->last = region;
		arena->first = region;
	}

	// deal with the zero case here specially to simplify the alignment-calculating code below.
	if (size == 0) {
		// anyway, we now know we have *a* region -- so it's valid to just return it.
		return arena->last->buffer + arena->last->size;
	}

	// alignment must be a power of two.
	assert((alignment & (alignment - 1)) == 0);

	Region *cur = arena->last;
	while (true) {

		char *ptr = (char*) (((uintptr_t) (cur->buffer + cur->size + (alignment - 1))) & ~(alignment - 1));
		size_t real_size = (size_t) ((ptr + size) - (cur->buffer + cur->size));

		if (cur->size + real_size > cur->capacity) {
			if (cur->next) {
				cur = cur->next;
				continue;
			} else {
				// out of space, make a new one. even though we are making a new region, there
				// aren't really any guarantees on the alignment of memory that malloc() returns.
				// so, allocate enough extra bytes to fix the 'worst case' alignment.
				size_t worst_case = size + (alignment - 1);

				Region *region = region_new(worst_case > ARENA_DEFAULT_CAPACITY
								   ? worst_case
								   : ARENA_DEFAULT_CAPACITY);

				arena->last->next = region;
				arena->last = region;
				cur = arena->last;

				// ok, now we know we have enough space. just go back to the top of the loop here,
				// so we don't duplicate the code. we now know that we will definitely succeed,
				// so there won't be any infinite looping here.
				continue;
			}
		} else {
			memset(ptr, 0, real_size);
			cur->size += real_size;
			return ptr;
		}
	}
}

void *arena_alloc(Arena *arena, size_t size) {
	// by default, align to a pointer size. this should be sufficient on most platforms.
	return arena_alloc_aligned(arena, size, sizeof(void*));
}

void *arena_realloc(Arena *arena, void *old_ptr, size_t old_size, size_t new_size) {
	if (old_size < new_size) {
		void *new_ptr = arena_alloc(arena, new_size);
		memcpy(new_ptr, old_ptr, old_size);
		return new_ptr;
	} else {
		return old_ptr;
	}
}

void arena_clean(Arena *arena) {
	for (Region *iter = arena->first;
	iter != NULL;
	iter = iter->next) {
		iter->size = 0;
	}

	arena->last = arena->first;
}

void arena_free(Arena *arena) {
	Region *iter = arena->first;
	while (iter != NULL) {
		Region *next = iter->next;
		free(iter);
		iter = next;
	}
	arena->first = NULL;
	arena->last = NULL;
}

void arena_summary(Arena *arena) {
	if (arena->first == NULL) {
		printf("[empty]");
	}

	for (Region *iter = arena->first;
	iter != NULL;
	iter = iter->next) {
		printf("[%zu/%zu] -> ", iter->size, iter->capacity);
	}
	printf("\n");
}







#include <stdio.h>

typedef enum ExpressionType {
	EXPR_ASSIGNMENT,
	EXPR_LOGICAL,
	EXPR_BINARY,
	EXPR_UNARY,
	EXPR_CHAIN,
	EXPR_LITERAL_INT,
	EXPR_LITERAL_FLOAT,
	EXPR_LITERAL_STRING,
	EXPR_VARIABLE,
	EXPR_GROUPING,
	EXPR_CALL,
} ExpressionType;


typedef struct Expression Expression;

typedef struct {
	Token name;
	Expression* value;
} ExpressionAssignment;

typedef struct {
	Expression* left;
	Token op;
	Expression* right;
} ExpressionLogical;

typedef struct {
	Expression* left;
	Token op;
	Expression* right;
} ExpressionBinary;

typedef struct {
	Token op;
	Expression* right;
} ExpressionUnary;

typedef struct {
	Expression* left;
	Expression* right;
} ExpressionChain;

typedef struct {
	int value;
} ExpressionLiteralInt;

typedef struct {
	float value;
} ExpressionLiteralFloat;

typedef struct {
	StringView str;
} ExpressionLiteralString;

typedef struct {
	Token name;
} ExpressionVariable;

typedef struct {
	Expression* expr;
} ExpressionGrouping;

typedef struct {
	Expression* callee;
	Token token;
	size_t argc;
	Expression** args;
} ExpressionCall;

struct Expression {
	ExpressionType type;
	union {
		ExpressionAssignment assignment;
		ExpressionLogical logical;
		ExpressionBinary binary;
		ExpressionUnary unary;
		ExpressionChain chain;
		ExpressionLiteralInt literal_int;
		ExpressionLiteralFloat literal_float;
		ExpressionLiteralString literal_string;
		ExpressionVariable variable;
		ExpressionGrouping grouping;
		ExpressionCall call;
	};
};


const char* expression_name_cstr(ExpressionType et);

void expression_print(Expression* expr, int indent);





typedef enum StatementType {
	STATEMENT_EXPRESSION,
	STATEMENT_BLOCK,
	STATEMENT_DESCRIPTION,
} StatementType;


typedef struct Statement Statement;


typedef struct {
	Expression* expression;
} StatementExpression;

typedef struct {
	size_t statement_count;
	Statement** statements;
} StatementBlock;

typedef struct {
	Statement* statement;
	Statement* block;
} StatementDescription;

struct Statement {
	StatementType type;
	union {
		StatementExpression expression;
		StatementBlock block;
		StatementDescription description;
	};
};

typedef struct {
	Statement** items;
	size_t count;
	size_t capacity;
} StatementList;


StatementList statement_list_new();

const char* statement_name_cstr(StatementType st);

void statement_print(Statement* s, int indent);


typedef struct {
	Lexer* lexer;
	Token current;
	Token next;
	Token previous;
	bool had_error;
	Arena arena;
} Parser;

Parser parser_new(Lexer* lexer);

StatementList parser_parse_all(Parser* p);

void parser_error_at_token(Parser* p, Token token, const char* error_cstr);

Expression* parser_arena_alloc_expression(Parser* p);
Statement*  parser_arena_alloc_statement (Parser* p);

bool  parser_is_at_end (Parser* p);
Token parser_advance   (Parser* p);
bool  parser_check     (Parser* p, TokenType type);
bool  parser_check_next(Parser* p, TokenType type);
bool  parser_match     (Parser* p, TokenType type);
Token parser_consume   (Parser* p, TokenType type, const char* err_cstr);

Expression* parse_expression (Parser* p);
Expression* parse_assignment (Parser* p);
Expression* parse_logical_or (Parser* p);
Expression* parse_logical_and(Parser* p);
Expression* parse_equality   (Parser* p);
Expression* parse_comparison (Parser* p);
Expression* parse_term       (Parser* p);
Expression* parse_factor     (Parser* p);
Expression* parse_unary      (Parser* p);
Expression* parse_call       (Parser* p);
Expression* parse_primary    (Parser* p);

Expression* parser_finish_call(Parser* p, Expression* callee);

int   parse_literal_int  (const char* str, size_t len);
float parse_literal_float(const char* str, size_t len);

Statement* parse_declaration         (Parser* p);
Statement* parse_statement           (Parser* p);
Statement* parse_block_statement     (Parser* p);
Statement* parse_expression_statement(Parser* p);

void parser_dump(Parser* p);






#include <stdio.h>


Parser parser_new(Lexer* lexer) {
	Parser p = {
		.lexer     = lexer,
		.had_error = false,
	};
	parser_advance(&p);
	parser_advance(&p);
	return p;
}


void parser_error_at_token(Parser* p, Token token, const char* error_cstr) {
	p->had_error = true;
	fprintf(stderr,"[ERROR][parser] %zu:%zu %s\n\t%s %.*s\n",
		 token.line, token.column,
		 error_cstr, token_name_cstr(token), (int)token.str.count, token.str.items);
}

Expression* parser_arena_alloc_expression(Parser* p) {
	return (Expression*)arena_alloc(&p->arena, sizeof(Expression));
}
Statement* parser_arena_alloc_statement(Parser* p) {
	return (Statement*)arena_alloc(&p->arena, sizeof(Statement));
}

bool parser_is_at_end(Parser* p) {
	return p->current.type == TOKEN_END;
}


Token parser_advance(Parser* p) {
	p->previous = p->current;
	p->current = p->next;
	p->next = lexer_next_token(p->lexer);
	return p->previous;
}

bool parser_check(Parser* p, TokenType type) {
	return p->current.type == type;
}
bool parser_check_next(Parser* p, TokenType type) {
	return p->next.type == type;
}
bool parser_match(Parser* p, TokenType type) {
	if (parser_check(p, type)) {
		parser_advance(p);
		return true;
	}
	return false;
}
Token parser_consume(Parser* p, TokenType type, const char* err_cstr) {
	if (parser_check(p, type)) {
		return parser_advance(p);
	}
	parser_error_at_token(p, p->previous, err_cstr);
	return parser_advance(p);
}




Expression* parse_expression(Parser* p) {
	return parse_assignment(p);
}

Expression* parse_assignment(Parser* p) {
	Expression* expression = parse_logical_or(p);

	if (parser_match(p, TOKEN_EQUAL)) {
		Token equals = p->previous;
		Expression* value = parse_assignment(p);
		if (expression->type == EXPR_VARIABLE) {
			Expression* assign = parser_arena_alloc_expression(p);
			assign->type = EXPR_ASSIGNMENT;
			assign->assignment.name = expression->variable.name;
			assign->assignment.value = value;
			return assign;
		} else {
			parser_error_at_token(p, equals, "Invalid assignment target.");
		}
	}

	return expression;
}



Expression* parse_logical_or(Parser* p) {
	Expression* expr = parse_logical_and(p);

	while (parser_match(p, TOKEN_OR_OR)) {
		Token op = p->previous;
		Expression* right = parse_logical_and(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_LOGICAL;
		e->logical.left = expr;
		e->logical.op = op;
		e->logical.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_logical_and(Parser* p) {
	Expression* expr = parse_equality(p);

	while (parser_match(p, TOKEN_AND_AND)) {
		Token op = p->previous;
		Expression* right = parse_equality(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_LOGICAL;
		e->logical.left = expr;
		e->logical.op = op;
		e->logical.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_equality(Parser* p) {
	Expression* expr = parse_comparison(p);

	while (parser_match(p, TOKEN_EXCLAMATION_EQUAL) || parser_match(p, TOKEN_EQUAL_EQUAL)) {
		Token op = p->previous;
		Expression* right = parse_comparison(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_BINARY;
		e->binary.left = expr;
		e->binary.op = op;
		e->binary.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_comparison(Parser* p) {
	Expression* expr = parse_term(p);

	while (parser_match(p, TOKEN_GREATER) || parser_match(p, TOKEN_GREATER_EQUAL)
		|| parser_match(p, TOKEN_LESS) || parser_match(p, TOKEN_LESS_EQUAL)) {
		Token op = p->previous;
		Expression* right = parse_term(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_BINARY;
		e->binary.left = expr;
		e->binary.op = op;
		e->binary.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_term(Parser* p) {
	Expression* expr = parse_factor(p);

	while (parser_match(p, TOKEN_MINUS) || parser_match(p, TOKEN_PLUS)) {
		Token op = p->previous;
		Expression* right = parse_factor(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_BINARY;
		e->binary.left = expr;
		e->binary.op = op;
		e->binary.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_factor(Parser* p) {
	Expression* expr = parse_unary(p);

	while (parser_match(p, TOKEN_PERCENT) || parser_match(p, TOKEN_SLASH) || parser_match(p, TOKEN_STAR)) {
		Token op = p->previous;
		Expression* right = parse_unary(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_BINARY;
		e->binary.left = expr;
		e->binary.op = op;
		e->binary.right = right;
		expr = e;
	}

	return expr;
}

Expression* parse_unary(Parser* p) {
	if (parser_match(p, TOKEN_EXCLAMATION) || parser_match(p, TOKEN_MINUS)
		|| parser_match(p, TOKEN_MINUS_MINUS) || parser_match(p, TOKEN_PLUS_PLUS)) {

		Token op = p->previous;
		Expression* right = parse_unary(p);

		Expression* e = parser_arena_alloc_expression(p);
		e->type = EXPR_UNARY;
		e->unary.op = op;
		e->unary.right = right;
		return e;
	}

	return parse_call(p);
}

Expression* parse_call(Parser* p) {
	Expression* expr = parse_primary(p);
	while (true) {
		if (parser_match(p, TOKEN_OPEN_PAREN)) {
			expr = parser_finish_call(p, expr);
		} else if (parser_match(p, TOKEN_DOT)) {
			Expression* right = parse_call(p);
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_CHAIN;
			e->chain.left = expr;
			e->chain.right = right;
			expr = e;
// 		} else if (parser_match(p, TOKEN_COLON) && parser_match(p, TOKEN_COLON)) {
// 			expr = parse_call(p);
		} else {
			break;
		}
	}
	return expr;
}

Expression* parse_primary(Parser* p) {
	parser_advance(p);
	switch (p->previous.type) {
		case TOKEN_IDENTIFIER: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_VARIABLE;
			e->variable.name = p->previous;
			return e;
		}
		case TOKEN_KEYWORD_FALSE: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_INT;
			e->literal_int.value = 0;
			return e;
		}
		case TOKEN_KEYWORD_TRUE: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_INT;
			e->literal_int.value = 1;
			return e;
		}
		case TOKEN_INTEGER_LITERAL: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_INT;
			e->literal_int.value = parse_literal_int(p->previous.str.items, p->previous.str.count);
			return e;
		}
		case TOKEN_FLOAT_LITERAL: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_FLOAT;
			e->literal_float.value = parse_literal_float(p->previous.str.items, p->previous.str.count);
			return e;
		}
		case TOKEN_STRING_LITERAL: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_STRING;
			e->literal_string.str = p->previous.str;
			return e;
		}
		case TOKEN_OPEN_PAREN: {
			Expression* expr = parse_expression(p);
			parser_consume(p, TOKEN_CLOSE_PAREN, "Expect ')' after expression.");
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_GROUPING;
			e->grouping.expr = expr;
			return e;
		}
		case TOKEN_END: {
			return NULL;
		}

		default: break;
	}
	parser_error_at_token(p, p->previous, "Expected expression.");
	return NULL;
}

Expression* parser_finish_call(Parser* p, Expression* callee) {
	Expression* args[64];
	size_t argc = 0;

	while (parser_match(p, TOKEN_COMMA) || !parser_check(p, TOKEN_CLOSE_PAREN)) {
		if (argc >= 64) {
			parser_error_at_token(p, p->previous, "Can't have more than 63 arguments.");
		}

		if (parser_match(p, TOKEN_DOLLAR)) {
			args[argc++] = parse_expression(p);
		} else if (parser_match(p, TOKEN_AT)) {
			//args[argc++] = parse_macro(p);
		} else {
			Token t = parser_advance(p);

			const char* start = t.str.items;
			while (!parser_check(p, TOKEN_CLOSE_PAREN)
				&& !parser_check(p, TOKEN_COMMA)
				&& !parser_check(p, TOKEN_DOLLAR)) {
				t = parser_advance(p);
			}
			const char* end = t.str.items + t.str.count;

			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_STRING;
			e->literal_string.str.items = start;
			e->literal_string.str.count = end - start;
			args[argc++] = e;
		}
	}

	Token paren = parser_consume(p, TOKEN_CLOSE_PAREN, "Expected ')' after arguments.");

	Expression* e = parser_arena_alloc_expression(p);
	e->type = EXPR_CALL;
	e->call.callee = callee;
	e->call.token = paren;
	e->call.argc = argc;

	e->call.args = (Expression**)arena_alloc(&p->arena, sizeof(Expression)*argc);
	for (size_t i = 0; i < argc; ++i) {
		e->call.args[i] = args[i];
	}

	return e;
}

int parse_literal_int(const char* str, size_t len) {
	return 0;
}
float parse_literal_float(const char* str, size_t len) {
	return 0;
}




StatementList parser_parse_all(Parser* p) {
	StatementList statement_list = statement_list_new();

	while (!parser_is_at_end(p)) {
		Statement* s = parse_declaration(p);
		da_append_arena(&p->arena, &statement_list, s);
	}

	return statement_list;
}

Statement* parse_declaration(Parser* p) {
	return parse_statement(p);
}
Statement* parse_statement(Parser* p) {
	return parse_expression_statement(p);
}

Statement* parse_block_statement(Parser* p) {
	StatementList statement_list = statement_list_new();

	while (!parser_check(p, TOKEN_CLOSE_CURLY) && !parser_is_at_end(p)) {
		Statement* s = parse_declaration(p);
		da_append_arena(&p->arena, &statement_list, s);
	}

	parser_consume(p, TOKEN_CLOSE_CURLY, "Expected '}' after block.");

	Statement* s = parser_arena_alloc_statement(p);
	s->type = STATEMENT_BLOCK;
	s->block.statement_count = statement_list.count;
	s->block.statements = statement_list.items;
	return s;
}
Statement* parse_expression_statement(Parser* p) {
	Expression* expr = parse_expression(p);
	Statement* s = parser_arena_alloc_statement(p);

	if (parser_match(p, TOKEN_OPEN_CURLY)) {
		Statement* b = parse_block_statement(p);

		Statement* se = parser_arena_alloc_statement(p);
		se->type = STATEMENT_EXPRESSION;
		se->expression.expression = expr;

		s->type = STATEMENT_DESCRIPTION;
		s->description.statement = se;
		s->description.block = b;
	} else {
		s->type = STATEMENT_EXPRESSION;
		s->expression.expression = expr;
	}

	parser_match(p, TOKEN_SEMICOLON);
	return s;
}


void parser_dump(Parser* p) {
	Parser copy_parser = *p;
	Lexer  copy_lexer  = *p->lexer;
	copy_parser.lexer = &copy_lexer;

// 	Expression* e = parse_expression(&copy_parser);
// 	while (e != NULL) {
// 		expression_print(e, 0);
// 		e = parse_expression(&copy_parser);
// 	}

	StatementList sl = parser_parse_all(&copy_parser);
	for (size_t i = 0; i < sl.count; ++i) {
		Statement* s = sl.items[i];
		statement_print(s, 1);
	}
}



const char* expression_name_cstr(ExpressionType et) {
	switch (et) {
		#define CASE(T) case T: return #T;
		CASE(EXPR_ASSIGNMENT)
		CASE(EXPR_LOGICAL)
		CASE(EXPR_BINARY)
		CASE(EXPR_UNARY)
		CASE(EXPR_CHAIN)
		CASE(EXPR_LITERAL_INT)
		CASE(EXPR_LITERAL_FLOAT)
		CASE(EXPR_LITERAL_STRING)
		CASE(EXPR_VARIABLE)
		CASE(EXPR_GROUPING)
		CASE(EXPR_CALL)
		#undef CASE
	}
	return "!!! EXPR INVALID";
}


inline static void print_indent(int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
}
void expression_print(Expression* expr, int indent) {
	if (!expr) {
		return;
	}

	print_indent(indent);

	switch(expr->type) {
		case EXPR_VARIABLE:
			printf("variable: %.*s\n", (int)expr->variable.name.str.count, expr->variable.name.str.items);
			break;
		case EXPR_LITERAL_INT:
			printf("literal int: %d\n", expr->literal_int.value);
			break;
		case EXPR_LITERAL_STRING:
			printf("literal string: '%.*s'\n", (int)expr->literal_string.str.count, expr->literal_string.str.items);
			break;
		case EXPR_CALL:
			printf("call:\n");
			print_indent(indent + 2);
			printf("method:\n");
			expression_print(expr->call.callee, indent + 3);
			print_indent(indent + 2);
			printf("arguments:\n");
			for (size_t i = 0; i < expr->call.argc; ++i) {
				expression_print(expr->call.args[i], indent + 4);
			}
			break;
		case EXPR_CHAIN:
			printf("chain:\n");
			print_indent(indent + 2);
			printf("left:\n");
			expression_print(expr->chain.left, indent + 3);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->chain.right, indent + 3);
			break;
		case EXPR_UNARY:
			printf("unary:\n");
			print_indent(indent + 2);
			printf("op: %.*s\n", (int)expr->unary.op.str.count, expr->unary.op.str.items);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->unary.right, indent + 3);
			break;
		case EXPR_BINARY:
			printf("binary:\n");
			print_indent(indent + 2);
			printf("left:\n");
			expression_print(expr->binary.left, indent + 3);
			print_indent(indent + 2);
			printf("op: %.*s\n", (int)expr->binary.op.str.count, expr->binary.op.str.items);
			print_indent(indent + 2);
			printf("right:\n");
			expression_print(expr->binary.right, indent + 3);
			break;
		case EXPR_GROUPING:
			printf("grouping:\n");
			expression_print(expr->grouping.expr, indent + 3);
			break;
		default:
			printf("unknown expression type: %s\n", expression_name_cstr(expr->type));
			break;
	}
}


StatementList statement_list_new() {
	StatementList statement_list = {
		.count = 0,
		.capacity = 0,
		.items = NULL,
	};
	return statement_list;
}

const char* statement_name_cstr(StatementType st) {
	switch (st) {
		#define CASE(T) case T: return #T;
		CASE(STATEMENT_EXPRESSION)
		CASE(STATEMENT_BLOCK)
		CASE(STATEMENT_DESCRIPTION)
		#undef CASE
	}
	return "!!! STATEMENT INVALID";
}

void statement_print(Statement* s, int indent) {
	if (!s) {
		return;
	}

	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}

	printf("%s\n", statement_name_cstr(s->type));

	switch (s->type) {
		case STATEMENT_EXPRESSION:
			expression_print(s->expression.expression, indent + 1);
			break;
		case STATEMENT_DESCRIPTION:
			statement_print(s->description.statement, indent + 2);
			statement_print(s->description.block, indent + 2);
			break;
		case STATEMENT_BLOCK:
			for (size_t i = 0; i < s->block.statement_count; ++i) {
				statement_print(s->block.statements[i], indent + 1);
			}
			break;
	}
}








typedef enum SymbolValueType {
	SYMBOL_VALUE_NIL = 0,
	SYMBOL_VALUE_INT,
	SYMBOL_VALUE_FLOAT,
	SYMBOL_VALUE_STRING,
	SYMBOL_VALUE_METHOD,
	SYMBOL_VALUE_BUILD_COMMAND,
} SymbolValueType;

typedef enum MethodType {
	METHOD_BUILD,
	METHOD_COMPILER,
	METHOD_INPUT,
	METHOD_CFLAGS,
	METHOD_LDFLAGS,
	METHOD_SOURCE_DIR,
	METHOD_OUTPUT_DIR,
	METHOD_INCLUDE_DIR,
	METHOD_LIBRARY_DIR,
	METHOD_LINK,
} MethodType;

typedef struct SymbolValue {
	SymbolValueType type;
	union {
		int integer;
		float floating;
		StringView string;
		struct BuildCommand* bc;
		MethodType method_type;
	};
} SymbolValue;

typedef struct SymbolEntry {
	StringView name;
	SymbolValue value;
} SymbolEntry;

typedef struct SymbolMap {
	// TODO: use hash map
	// string -> symbol
	SymbolEntry** items;
	size_t count;
	size_t capacity;
} SymbolMap;


const char* symbol_value_type_name_cstr(SymbolValueType);
void symbol_value_print(SymbolValue value, int indent);





#include <stdio.h>


typedef struct Target {
	StringView name;
	StringBuilder input_name;
	StringBuilder output_name;
} Target;

typedef struct TargetList {
	Target* items;
	size_t count;
	size_t capacity;
} TargetList;


typedef enum BuildType {
	BUILD_EXECUTABLE,
	BUILD_OBJECT,
	BUILD_LIB,
} BuildType;

typedef struct BuildCommand BuildCommand;

typedef struct BuildCommandChildren {
	BuildCommand** items;
	size_t count;
	size_t capacity;
} BuildCommandChildren;

struct BuildCommand {
	BuildCommand* parent;

	BuildCommandChildren children;

	StringView compiler;

	BuildType build_type;

	size_t target_to_build;
	TargetList targets;

	StringList input_files;

	StringList include_dirs;
	StringList include_files;

	StringList library_dirs;
	StringList library_links;

	StringList cflags;
	StringList ldflags;

	StringView source_dir;
	StringView output_dir;
};

BuildCommand* build_command_new(Arena*);

BuildCommand build_command_default();
void         build_command_print  (BuildCommand* bc, size_t indent);
void         build_command_execute(BuildCommand* bc);
void         build_command_dump   (BuildCommand* bc, FILE* stream);

BuildCommand* build_command_inherit(Arena* arena, BuildCommand* parent);

void build_type_print(BuildType type);




typedef struct Environment {
	struct Environment* enclosing;
	SymbolMap map;
} Environment;


typedef struct {
	Parser* parser;
	bool had_error;
	Arena arena;
	Environment* current_environment;
	BuildCommand* current_build_command;
	int verbose;
} Interpreter;

Environment* environment_new(Arena*);
Interpreter interpreter_new(Parser*);

void interpreter_interpret(Interpreter*);
void interpreter_dry_run  (Interpreter*);

BuildCommand* interpreter_interpret_build_command(Interpreter*);

void interpreter_error(Interpreter* in, Token token, const char* error_cstr);

SymbolValue interpreter_evaluate (Interpreter* in, Expression* e);
SymbolValue interpreter_execute  (Interpreter* in, Statement*  s);
SymbolValue interpret_block      (Interpreter* in, StatementBlock*  s);
SymbolValue interpret_call       (Interpreter* in, ExpressionCall*  e);
SymbolValue interpret_chain      (Interpreter* in, ExpressionChain* e);
SymbolValue interpret_description(Interpreter* in, StatementDescription* s);

SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e);


SymbolValue interpret_method_build(Interpreter* in, ExpressionCall* e);

void interpreter_expand_build_command_targets(Interpreter* in, BuildCommand* bc);







#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const SymbolValue nil = { .type = SYMBOL_VALUE_NIL };


// TODO: define some constants
// COOK_VERSION=0.0.1
// GCC_VERSION (get it from gcc itself) ...
// DEBUG/RELEASE/MIN_SIZE_REL/DIST
Environment* environment_new(Arena* arena) {
	Environment* env = (Environment*)arena_alloc(arena, sizeof(Environment));
	return env;
}

Interpreter interpreter_new(Parser* p) {
	Interpreter in = {
		.parser = p,
	};
	in.current_environment = environment_new(&in.arena);
	in.current_build_command = build_command_new(&in.arena);
	return in;
}


void interpreter_interpret(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	build_command_execute(bc);
}

void interpreter_dry_run(Interpreter* in) {
	BuildCommand* bc = interpreter_interpret_build_command(in);
	if (in->verbose > 0) {
		printf("[interpreter] build command pretty:\n");
		build_command_print(bc, 0);
		printf("[interpreter] build command dump:\n");
	}
	build_command_dump(bc, stdout);
}

BuildCommand* interpreter_interpret_build_command(Interpreter* in) {
	StatementList sl = parser_parse_all(in->parser);
	for (size_t i = 0; i < sl.count; ++i) {
		interpreter_execute(in, sl.items[i]);
	}
	interpreter_expand_build_command_targets(in, in->current_build_command);
	return in->current_build_command;
}

void interpreter_error(Interpreter* in, Token token, const char* error_cstr) {
	in->had_error = true;
	fprintf(stderr,"[ERROR][interpreter] %zu:%zu %s\n\t%s %.*s\n",
		 token.line + 1, token.column,
		 error_cstr, token_name_cstr(token), (int)token.str.count, token.str.items);
	raise(1);
}



SymbolValue interpreter_evaluate(Interpreter* in, Expression* e) {
	if (!e) { return (SymbolValue){0}; }

	switch (e->type) {
		case EXPR_VARIABLE: return interpreter_lookup_variable(in, e->variable.name.str, e);
		case EXPR_CALL:     return interpret_call(in, &e->call);
		case EXPR_CHAIN:    return interpret_chain(in, &e->chain);
		case EXPR_LITERAL_STRING: {
			return (SymbolValue){
				.type = SYMBOL_VALUE_STRING,
				.string = e->literal_string.str,
			};
		}
		default: break;
	}
	return (SymbolValue){0};
}

SymbolValue interpreter_execute(Interpreter* in, Statement* s) {
	switch (s->type) {
		case STATEMENT_BLOCK:       return interpret_block(in, &s->block);
		case STATEMENT_DESCRIPTION: return interpret_description(in, &s->description);
		case STATEMENT_EXPRESSION:  return interpreter_evaluate(in, s->expression.expression);
	}
	return nil;
}

SymbolValue interpret_block(Interpreter* in, StatementBlock*  s) {
	assert(false);
	return nil;
}

SymbolValue interpret_description(Interpreter* in, StatementDescription* s) {
	BuildCommand* enclosing = in->current_build_command;

	SymbolValue left = interpreter_execute(in, s->statement);

	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		in->current_build_command = left.bc;
	}

	for (size_t i = 0; i < s->block->block.statement_count; ++i) {
		interpreter_execute(in, s->block->block.statements[i]);
	}

	in->current_build_command = enclosing;
	return left;
}

SymbolValue interpret_chain(Interpreter* in, ExpressionChain* e) {
	SymbolValue left = interpreter_evaluate(in, e->left);
	BuildCommand* enclosing = in->current_build_command;
	if (left.type == SYMBOL_VALUE_BUILD_COMMAND) {
		in->current_build_command = left.bc;
	}
	interpreter_evaluate(in, e->right);
	in->current_build_command = enclosing;
	return left;
}

// NOTE: how do we know when to exit the current build command ?
// desc     -> {   build()  .input() ... }
// chain    -> {   build()  .input() ... }
//             {   build()  .input() ... }
// no chain -> { { build() } input() ... }

SymbolValue interpret_call(Interpreter* in, ExpressionCall* e) {
	SymbolValue callee = interpreter_evaluate(in, e->callee);
	Token callee_token = {
		.type = TOKEN_IDENTIFIER,
		.str.count = callee.string.count,
		.str.items = callee.string.items,
		.line = e->token.line,
		.column = e->token.column,
	};

	if (callee.type == SYMBOL_VALUE_NIL) {
		interpreter_error(in, callee_token, "callee is nil");
		return nil;
	}
	
	if (callee.type != SYMBOL_VALUE_METHOD) {
		interpreter_error(in, callee_token, "callee is not a method");
		return nil;
	}

	// TODO: maybe arity check

	// append it to the current build context
	if (callee.method_type == METHOD_BUILD) {
		return interpret_method_build(in, e);
	} else if (callee.method_type == METHOD_INPUT) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->input_files, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_COMPILER) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "compiler method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->compiler = arg.string;
	} else if (callee.method_type == METHOD_CFLAGS) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->cflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LDFLAGS) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->ldflags, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_SOURCE_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "source_dir method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->source_dir = arg.string;
	} else if (callee.method_type == METHOD_OUTPUT_DIR) {
		if (e->argc != 1) {
			interpreter_error(in, e->token, "output_dir method takes only 1 argument");
			return nil;
		}
		BuildCommand* bc = in->current_build_command;
		SymbolValue arg = interpreter_evaluate(in, e->args[0]);
		bc->output_dir = arg.string;
	} else if (callee.method_type == METHOD_INCLUDE_DIR) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->include_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LIBRARY_DIR) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_dirs, arg.string);
			}
		}
	} else if (callee.method_type == METHOD_LINK) {
		BuildCommand* bc = in->current_build_command;
		for (size_t i = 0; i < e->argc; ++i) {
			SymbolValue arg = interpreter_evaluate(in, e->args[i]);
			if (arg.type == SYMBOL_VALUE_STRING) {
				da_append_arena(&in->arena, &bc->library_links, arg.string);
			}
		}
	}
	return nil;
}


SymbolValue interpreter_lookup_variable(Interpreter* in, StringView sv, Expression* e) {
	SymbolValue val = {0};
	val.string.items = sv.items;
	val.string.count = sv.count;
	val.type = SYMBOL_VALUE_STRING;

	#define METHOD(name, enm)                         \
		if (strncmp(name, sv.items, sv.count) == 0) { \
			val.type = SYMBOL_VALUE_METHOD;           \
			val.method_type = enm;                    \
			return val; }

	METHOD("build",       METHOD_BUILD);
	METHOD("compiler",    METHOD_COMPILER);
	METHOD("input",       METHOD_INPUT);
	METHOD("cflags",      METHOD_CFLAGS);
	METHOD("ldflags",     METHOD_LDFLAGS);
	METHOD("source_dir",  METHOD_SOURCE_DIR);
	METHOD("output_dir",  METHOD_OUTPUT_DIR);
	METHOD("include_dir", METHOD_INCLUDE_DIR);
	METHOD("library_dir", METHOD_LIBRARY_DIR);
	METHOD("link",        METHOD_LINK);

	return val;
}

SymbolValue interpret_method_build(Interpreter* in, ExpressionCall* e) {
	BuildCommand* enclosing = in->current_build_command;
	BuildCommand* bc = build_command_inherit(&in->arena, in->current_build_command);
	in->current_build_command = bc;

	for (size_t i = 0; i < e->argc; ++i) {
		SymbolValue arg = interpreter_evaluate(in, e->args[i]);
		if (arg.type == SYMBOL_VALUE_STRING) {
			Target t = { .name = arg.string };
			da_append_arena(&in->arena, &bc->targets, t);
		}
	}

	in->current_build_command = enclosing;

	return (SymbolValue){
		.type = SYMBOL_VALUE_BUILD_COMMAND,
		.bc = bc
	};
}

// NOTE: we have to wait for all the descriptions to end to run this,
// otherwise we might miss the compiler change
void interpreter_expand_build_command_targets(Interpreter* in, BuildCommand* bc) {
	for (size_t i = 0; i < bc->targets.count; ++i) {
		Target* t = &bc->targets.items[i];

		t->input_name.count = 0;
		if (bc->source_dir.count > 0) {
			da_append_many_arena(&in->arena, &t->input_name, bc->source_dir.items, bc->source_dir.count);
			da_append_arena(&in->arena, &t->input_name, '/');
		}
		da_append_many_arena(&in->arena, &t->input_name, t->name.items, t->name.count);
		if ((bc->compiler.count == 3 && strncmp(bc->compiler.items, "gcc", 3) == 0) ||
			(bc->compiler.count == 5 && strncmp(bc->compiler.items, "clang", 5) == 0)
		) {
			da_append_many_arena(&in->arena, &t->input_name, ".c", 2);
		} else if (bc->compiler.count == 3 && strncmp(bc->compiler.items, "g++", 3) == 0) {
			da_append_many_arena(&in->arena, &t->input_name, ".cpp", 4);
		}

		t->output_name.count = 0;

		if (bc->output_dir.count > 0) {
			da_append_many_arena(&in->arena, &t->output_name, bc->output_dir.items, bc->output_dir.count);
			da_append_arena(&in->arena, &t->output_name, '/');
		}
		da_append_many_arena(&in->arena, &t->output_name, t->name.items, t->name.count);
		if (bc->build_type == BUILD_OBJECT) {
			da_append_many_arena(&in->arena, &t->output_name, ".o", 2);
		}

		if (bc->parent) {
			da_append_arena(&in->arena, &bc->parent->input_files, sv_from_sb(t->output_name));
		}
	}

	for (size_t i = 0; i < bc->children.count; ++i) {
		interpreter_expand_build_command_targets(in, bc->children.items[i]);
	}
}


#include <stdio.h>

const char* symbol_value_type_name_cstr(SymbolValueType type) {
	switch (type) {
		#define CASE(T) case T: return #T;
		CASE(SYMBOL_VALUE_NIL)
		CASE(SYMBOL_VALUE_INT)
		CASE(SYMBOL_VALUE_FLOAT)
		CASE(SYMBOL_VALUE_STRING)
		CASE(SYMBOL_VALUE_METHOD)
		CASE(SYMBOL_VALUE_BUILD_COMMAND)
		#undef CASE
	}
	return "!!! SYMBOL VALUE TYPE INVALID";
}

void symbol_value_print(SymbolValue value, int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
	printf("symbol ");
	switch (value.type) {
		case SYMBOL_VALUE_NIL:    printf("nil"); break;
		case SYMBOL_VALUE_INT:    printf("int: %d", value.integer); break;
		case SYMBOL_VALUE_FLOAT:  printf("float: %f", value.floating); break;
		case SYMBOL_VALUE_STRING: printf("string: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_METHOD: printf("method: %.*s", (int)value.string.count, value.string.items); break;
		case SYMBOL_VALUE_BUILD_COMMAND: printf("build command:\n\t\t"); build_command_print(value.bc, indent + 2); break;
		default: break;
	}
	printf("\n");
}



#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define INDENT_MULTIPLIER 4
#define CMD_LINE_MAX 2000

BuildCommand* build_command_new(Arena* arena) {
	BuildCommand* bc = (BuildCommand*)arena_alloc(arena, sizeof(BuildCommand));
	*bc = build_command_default();
	return bc;
}

BuildCommand build_command_default() {
	static const StringView gcc = { .items = "gcc", .count = 3 };

	BuildCommand bc = {0};
	bc.compiler = gcc;
	bc.build_type = BUILD_EXECUTABLE;
	return bc;
}

BuildCommand* build_command_inherit(Arena* arena, BuildCommand* parent) {
	if (!parent) {
		return NULL;
	}
	BuildCommand* bc = build_command_new(arena);

	bc->parent = parent;
	bc->compiler = parent->compiler;
	bc->include_dirs = parent->include_dirs;
	bc->library_dirs = parent->library_dirs;
	bc->cflags = parent->cflags;
	bc->ldflags = parent->ldflags;
	bc->source_dir = parent->source_dir;
	bc->output_dir = parent->output_dir;

	if (parent->parent != NULL) {
		bc->build_type = BUILD_OBJECT;
	}

	da_append_arena(arena, &parent->children, bc);
	return bc;
}


inline static void indent_label(int indent, const char* label) {
	printf("%*s%-14s: ", indent * INDENT_MULTIPLIER, "", label);
}
inline static void string_list_print_big(int indent, const char* label, const StringList* list) {
	if (list->count == 0) return;

	indent_label(indent, label);
	for (size_t i = 0; i < list->count; ++i) {
		const StringView* sv = &list->items[i];
		printf("%.*s", (int)sv->count, sv->items);
		if (i + 1 < list->count) {
			printf(", ");
		}
	}
	printf("\n");
}
inline static void target_list_print_big(int indent, const char* label, const TargetList* list) {
	if (list->count == 0) return;

	indent_label(indent, label);
	for (size_t i = 0; i < list->count; ++i) {
		const Target* sv = &list->items[i];
		printf("%.*s", (int)sv->name.count, sv->name.items);
		if (i + 1 < list->count) {
			printf(", ");
		}
	}
	printf("\n");
}

void build_command_print(BuildCommand* bc, size_t indent) {
	if (!bc) {
		return;
	}
	size_t ni = indent + 1;

	indent_label(ni, "build command");
	printf("\n");


	if (bc->compiler.count > 0) {
		indent_label(ni, "compiler");
		printf("%.*s\n", (int)bc->compiler.count, bc->compiler.items);
	}

	indent_label(ni, "build type");
	build_type_print(bc->build_type);
	printf("\n");

	target_list_print_big(ni, "targets", &bc->targets);
	string_list_print_big(ni, "input files", &bc->input_files);
	string_list_print_big(ni, "include dirs", &bc->include_dirs);
	string_list_print_big(ni, "include files", &bc->include_files);
	string_list_print_big(ni, "library dirs", &bc->library_dirs);
	string_list_print_big(ni, "library links", &bc->library_links);
	string_list_print_big(ni, "cflags", &bc->cflags);
	string_list_print_big(ni, "ldflags", &bc->ldflags);

	if (bc->source_dir.count > 0) {
		indent_label(ni, "source dir");
		printf("%.*s\n", (int)bc->source_dir.count, bc->source_dir.items);
	}
	if (bc->output_dir.count > 0) {
		indent_label(ni, "output dir");
		printf("%.*s\n", (int)bc->output_dir.count, bc->output_dir.items);
	}

	if (bc->children.count > 0) {
		indent_label(ni, "children");
		printf("%zu\n", bc->children.count);
		for (size_t i = 0; i < bc->children.count; ++i) {
			indent_label(indent + 2, "child");
			printf("%zu\n", i);
			build_command_print(bc->children.items[i], indent + 2);
		}
	}
}


inline static void print_stringview(FILE* stream, StringView sv) {
	fprintf(stream, "%.*s ", (int)sv.count, sv.items);
}

inline static void string_list_print_flat(FILE* stream, const StringList* list, const char* prefix) {
	if (!list || !list->items) {
		return;
	}
	for (size_t i = 0; i < list->count; ++i) {
		if (prefix) { fprintf(stream, "%s", prefix); }
		print_stringview(stream, list->items[i]);
	}
}

void build_command_dump(BuildCommand* bc, FILE* stream) {
	if (!bc) {
		return;
	}

	// skip root bc
	if (!bc->parent) {
		for (size_t i = 0; i < bc->children.count; ++i) {
			build_command_dump(bc->children.items[i], stream);
		}
		return;
	}

	// recurse into children
	for (size_t i = 0; i < bc->children.count; ++i) {
		build_command_dump(bc->children.items[i], stream);
	}

	if (bc->targets.count == 0) {
		return;
	}
	if (bc->targets.count <= bc->target_to_build) {
		return;
	}


	if (bc->compiler.count > 0) {
		print_stringview(stream, bc->compiler);
	}

	string_list_print_flat(stream, &bc->cflags, "");

	if (bc->build_type == BUILD_OBJECT) {
		fprintf(stream, "-c ");
	}

	fprintf(stream, "-o ");
	Target* t = &bc->targets.items[bc->target_to_build];
	print_stringview(stream, sv_from_sb(t->output_name));
	print_stringview(stream, sv_from_sb(t->input_name));

	string_list_print_flat(stream, &bc->include_dirs, "-I");
	string_list_print_flat(stream, &bc->input_files, "");
	string_list_print_flat(stream, &bc->library_dirs, "-L");
	string_list_print_flat(stream, &bc->library_links, "-l");
	string_list_print_flat(stream, &bc->ldflags, "");

	fprintf(stream, "\n");

	if (bc->target_to_build == 0) {
		for (size_t i = 1; i < bc->targets.count; ++i) {
			bc->target_to_build = i;
			build_command_dump(bc, stream);
		}
	}
}


static inline int execute_line(const char* line) {
#ifdef _WIN32
		char full_command[CMD_LINE_MAX + 16];
		snprintf(full_command, sizeof(full_command), "cmd /C \"%s\"", line);
		return system(full_command);
#else
	return system(line);
#endif
}


void build_command_execute(BuildCommand* bc) {
	// create output dir
	{
		StringBuilder sb = {0};
		da_append_many(&sb, bc->output_dir.items, bc->output_dir.count);
		da_append(&sb,'\0');
		if (access(sb.items, F_OK) != 0) {
			MKDIR(sb.items);
		}
		free(sb.items);
	}

	FILE* script = tmpfile();
	if (!script) {
		perror("tmpfile");
		exit(1);
	}

	build_command_dump(bc, script);

	rewind(script);
	char line[CMD_LINE_MAX];
	while (fgets(line, sizeof(line), script)) {
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}
		printf("> %.*s\n", (int)len, line);
		int ret = execute_line(line);
		if (ret != 0) {
			break;
		}
	}

	fclose(script);
}

void build_type_print(BuildType type) {
	switch (type) {
		case BUILD_EXECUTABLE: printf("executable"); return;
		case BUILD_OBJECT:     printf("object");     return;
		case BUILD_LIB:        printf("lib");        return;
	}
}



#include <stdbool.h>

typedef struct CookOptions {
	StringView source;
	bool dry_run;
	int verbose;
} CookOptions;

static inline CookOptions cook_options_default() {
	return (CookOptions){
		.dry_run = false,
		.verbose = 0,
	};
}

int cook(CookOptions op);





int cook(CookOptions op) {
	Lexer lexer = lexer_new(op.source);
	if (op.verbose > 2) {
		printf("[lexer] dump:\n");
		lexer_dump(&lexer);
	}
	Parser parser = parser_new(&lexer);
	if (op.verbose > 1) {
		printf("[parser] dump:\n");
		parser_dump(&parser);
	}
	Interpreter interpreter = interpreter_new(&parser);

	interpreter.verbose = op.verbose;

	if (op.dry_run) {
		interpreter_dry_run(&interpreter);
	} else {
		interpreter_interpret(&interpreter);
	}

	return 0;
}



#include <stdio.h>
#include <unistd.h>

void print_usage(const char* pname) {
	fprintf(stderr,
		"cook - better make\n"
		"usage: %s [options]\n"
		"\n"
		"options:\n"
		"  -h, --help      show this help message\n"
		"  -f <file>       use specified cookfile\n"
		"  --verbose       verbose printing\n"
		"  --dry-run       show the commands that would be run, but don't execute them\n",
		pname
	);
}

#define shift(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)

int main(int argc, char** argv) {
	CookOptions op = cook_options_default();

	const char* pname = shift(argv, argc);
	const char* filepath = NULL;

	while (argc > 0) {
		const char* arg = shift(argv, argc);

		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage(pname);
			return 0;
		} else if (strcmp(arg, "-f") == 0) {
			if (argc == 0) {
				fprintf(stderr, "[ERROR] expected a filepath after -f\n");
				print_usage(pname);
				return 1;
			}
			filepath = shift(argv, argc);
		} else if (strcmp(arg, "--dry-run") == 0) {
			op.dry_run = true;
		} else if (strncmp(arg, "--verbose=", 10) == 0) {
			op.verbose = arg[10] - '0';
		} else if (strcmp(arg, "--verbose") == 0) {
			op.verbose = 1;
		} else {
			fprintf(stderr, "[ERROR] unrecognized argument: %s\n", arg);
			print_usage(pname);
			return 1;
		}
	}

	StringBuilder source = {0};

	if (filepath) {
		if (!read_entire_file(filepath, &source)) {
			return 1;
		}
		op.source = sv_from_sb(source);
	} else if (access("./Cookfile", F_OK) == 0 && read_entire_file("./Cookfile", &source)) {
		op.source = sv_from_sb(source);
	} else {
		print_usage(pname);
		return 1;
	}

	bool result = cook(op);

	sb_free(&source);
	return result;
}
