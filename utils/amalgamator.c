/* amalgamator
 *
 * usage: amalgamator <source_files> -I <include_dirs> -o <output_file>
 *
 * hanion.dev
 */

#include <unistd.h>
#include <getopt.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
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

#define da_append_cstr(da, cstr) da_append_many((da), (cstr), strlen((cstr)))
#define da_append_sv(da, sv)     da_append_many((da), (sv)->items, (sv)->count)
#define da_append_token(da, tok) da_append_sv  ((da), &(tok).text)

typedef struct {
	char *items;
	size_t count;
	size_t capacity;
} StringBuilder;

typedef struct {
	char *items;
	size_t count;
} StringView;


/* lexer */

typedef enum {
	TOKEN_END = 0,
	TOKEN_INVALID,
	TOKEN_DONT_CARE,
	TOKEN_NEWLINE,
	TOKEN_COMMENT,
	TOKEN_LITERAL,
	TOKEN_INTEGER,
	TOKEN_SYMBOL,
} TokenType;

typedef struct {
	TokenType type;
	StringView text;
	bool preproc_end;
} Token;

typedef struct {
	const char* content;
	size_t content_length;
	size_t cursor;
	bool preprocessor_mode;
	bool preprocessor_in_string;
} Lexer;



bool is_delimiter(char c);
bool is_operator(char c);
bool is_integer(char c);

bool is_symbol_start(char c);
bool is_symbol(char c);


Lexer lexer_new(StringBuilder sb);

char lexer_advance(Lexer* l);
bool lexer_match_next(Lexer* l, char c);
bool lexer_match_string(Lexer* l, const char* str, size_t len);
void lexer_trim_left(Lexer* l);
void lexer_skip_until_new_line(Lexer* l);


Token lexer_next(Lexer* l);


bool is_delimiter(char c) {
	return (c == '+' || c == '-'
		|| c == '*' || c == '/' || c == ','
		|| c == ';' || c == '%' || c == '>'
		|| c == '<' || c == '=' || c == '('
		|| c == ')' || c == '[' || c == ']'
		|| c == '{' || c == '}');
}

bool is_operator(char c) {
	return (c == '+' || c == '-' || c == '*'
		|| c == '/' || c == '>' || c == '<'
		|| c == '=');
}

bool is_integer(char c) {
	return (c >= '0' && c <= '9');
}

bool is_symbol_start(char c) {
	return (unsigned char)c >= 128 || isalpha(c) || c == '_';
}

bool is_symbol(char c) {
	return (unsigned char)c >= 128 || isalnum(c) || c == '_';
}


Lexer lexer_new(StringBuilder sb) {
	Lexer lexer = {
		.content = sb.items,
		.content_length = sb.count,
		.cursor = 0,
		.preprocessor_mode = false,
		.preprocessor_in_string = false
	};
	return lexer;
}

char lexer_advance(Lexer* l) {
	assert(l->cursor < l->content_length);
	char c = l->content[l->cursor];
	l->cursor++;
	return c;
}

bool lexer_match(Lexer* l, char c) {
	if (l->cursor < l->content_length) {
		return c == l->content[l->cursor];
	}
	return false;
}

bool lexer_match_next(Lexer* l, char c) {
	if (l->cursor + 1 < l->content_length) {
		return c == l->content[l->cursor + 1];
	}
	return false;
}

bool lexer_match_string(Lexer* l, const char* str, size_t len) {
	size_t i = 0;
	while (l->cursor+i < l->content_length && i < len) {
		if (l->content[l->cursor+i] != str[i]) {
			return false;
		}
		++i;
	}
	return true;
}

// does not trim newline
void lexer_trim_left(Lexer* l) {
	while (l->cursor < l->content_length && isspace(l->content[l->cursor]) && l->content[l->cursor] != '\n') {
		lexer_advance(l);
	}
}
void lexer_skip_until_new_line(Lexer* l) {
	while (l->cursor < l->content_length && l->content[l->cursor] != '\n') {
		lexer_advance(l);
	}
}


Token lexer_next(Lexer* l) {
	lexer_trim_left(l);

	Token token = {
		.type = TOKEN_END,
		.text = {
			.items = (char*)&l->content[l->cursor],
			.count = 0
		},
		.preproc_end = false
	};
	token.text.items = (char*)&l->content[l->cursor];
	token.text.count = 0;

	if (l->cursor >= l->content_length) {
		return token;
	}

	if (lexer_match(l, '\n')) {
		lexer_advance(l);
		token.type = TOKEN_NEWLINE;
		token.text.count = 1;
		// TODO: handle '\'
		if (l->preprocessor_mode) {
			token.preproc_end = true;
			l->preprocessor_mode = false;
			l->preprocessor_in_string = false;
		}
		return token;
	}

	if (lexer_match(l, '/')) {
		size_t start = l->cursor;
		if (lexer_match_next(l, '/')) {
			lexer_skip_until_new_line(l);
			token.text.count = l->cursor-start;
			token.type = TOKEN_COMMENT;
			return token;
		}
	}

	if (lexer_match(l, '#')) {
		lexer_advance(l);
		l->preprocessor_mode = true;
		// 'ekle'
		Token next = lexer_next(l);
		// add '#'
		next.text.items = token.text.items;
		next.text.count++;
		return next;
	}

	if (l->preprocessor_mode) {
		if (lexer_match(l, '>')) {
			lexer_advance(l);
			token.type = TOKEN_DONT_CARE;
			token.text.count = 1;
			return token;
		}
		if (lexer_match(l, '"')) {
			lexer_advance(l);
			token.type = TOKEN_DONT_CARE;
			token.text.count = 1;
			if (!l->preprocessor_in_string) {
				l->preprocessor_in_string = true;
				return token;
			}
			l->preprocessor_in_string = false;
			return token;
		}
	}

	if (!l->preprocessor_mode && lexer_match(l, '"')) {
		lexer_advance(l);
		token.type = TOKEN_LITERAL;
		token.text.count++;
		while (l->cursor < l->content_length) {
			if (lexer_match(l, '\n') || lexer_match(l, '"')) {
				break;
			}
			token.text.count++;
			l->cursor++; ;
		}
		return token;
	}

	if (is_symbol_start(l->content[l->cursor])) {
		token.type = TOKEN_SYMBOL;
		while (l->cursor < l->content_length) {
			if (!is_symbol(l->content[l->cursor])) {
				if (l->preprocessor_mode && (lexer_match(l, '.') || lexer_match(l, '/'))) {
					// the '.' in #ekle
				} else {
					break;
				}
			}
			l->cursor++;
			token.text.count++;
		}
		return token;
	}

	char c = l->content[l->cursor];
	if (is_delimiter(c) || is_operator(c) || is_integer(c)) {
		token.type = is_integer(c) ? TOKEN_INTEGER : TOKEN_DONT_CARE;
		while (l->cursor < l->content_length) {
			if (lexer_match(l, '/') && lexer_match_next(l, '/')) {
				break;
			}
			char current = l->content[l->cursor];
			if (!is_delimiter(current) && !is_operator(current) && !is_integer(current)) {
				break;
			}
			l->cursor++;
			token.text.count++;
		}
		return token;
	}

	// unrecognized string
	l->cursor++;
	token.text.count = 1;
	token.type = TOKEN_INVALID;
	return token;
}
/* lexer */

/* file */
bool read_entire_file(const char *path, StringBuilder *sb) {
	bool result = true;

	FILE *f = fopen(path, "rb");
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
	if (!result) { printf("Could not read file %s: %s\n", path, strerror(errno)); }
	if (f) { fclose(f); }
	return result;
}

bool write_to_file(const char *path, StringBuilder *sb) {
	FILE *f = fopen(path, "wb");
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
/* file */


typedef struct {
	char **items;
	size_t count;
	size_t capacity;
} IncludedFiles;

extern char *strdup(const char *s);
void included_files_add(IncludedFiles *included_files, const char *file) {
	char* dup = strdup(file);
	da_append(included_files, dup);
}
bool included_files_contains(IncludedFiles *included_files, const char *file) {
	int len = strlen(file);
	for (size_t i = 0; i < included_files->count; i++) {
		if (strncmp(included_files->items[i], file, len) == 0) {
			return true;
		}
	}
	return false;
}

#define MAX_INPUTS 128
typedef struct {
	StringBuilder output;
	IncludedFiles included_files;
	char * source_files[MAX_INPUTS];
	char * include_dirs[MAX_INPUTS];
	size_t source_files_count;
	size_t include_dirs_count;
	const char* output_file;
} Amalgamator;

bool find_file_path(Amalgamator* a, char* filename, StringBuilder* out_fp) {
	if (access(filename, F_OK) == 0) {
		out_fp->items = strdup(filename);
		out_fp->count = strlen(filename);
		out_fp->capacity = out_fp->count;
		return true;
	}
	int including_from_dir = 0;
	while (access(out_fp->items, F_OK) != 0) {
		if (including_from_dir >= a->include_dirs_count) {
			return false;
		}
		const char* id = a->include_dirs[including_from_dir++];
		out_fp->count = 0;
		da_append_many(out_fp, id, strlen(id));
		da_append(out_fp, '/');
		da_append_many(out_fp, filename, strlen(filename) + 1); // +1 is the null terminator
	}
	return true;
}

bool process_file(Amalgamator* a, const char *file) {
	StringBuilder source = {0};
	if (!read_entire_file(file, &source)) {
		perror("read_entire_file error");
		return false;
	}

	Lexer lexer = lexer_new(source);
	Token token = lexer_next(&lexer);

	size_t cursor = 0;
	bool pack_tight = false;

	while (token.type != TOKEN_END && cursor < lexer.content_length) {
		if (!pack_tight) {
			// catch up to cursor
			while (cursor < lexer.content_length && &(lexer.content[cursor]) != token.text.items) {
				da_append(&a->output, lexer.content[cursor]);
				cursor++;
			}
		}

		if (lexer.preprocessor_mode && token.type == TOKEN_SYMBOL && strncmp(token.text.items, "#include", 8)==0) {
			// token is "#include"
			lexer_next(&lexer); // " or <
			Token include_text = lexer_next(&lexer); // the string

			// null terminate include_text.text
			StringBuilder filename = {0};
			da_append_token(&filename, include_text);
			da_append(&filename, '\0');

			StringBuilder found_fp = {0};
			bool included = false;
			if (find_file_path(a, filename.items, &found_fp)) {
				if (!included_files_contains(&a->included_files, found_fp.items)) {
					included_files_add(&a->included_files, found_fp.items);
					if (!process_file(a, found_fp.items)) {
						printf("failed to process file %.*s\n", (int)filename.count, filename.items);
						return false;
					}
				}
				included = true;
			}
			free(filename.items);

			// TODO: we are not handling '\' before newline anywhere
			// catch up to new line
			lexer_skip_until_new_line(&lexer);
			Token nl = lexer_next(&lexer);

			//assert(nl.preproc_end && "we assumed newline would be the end of the include");
			if (!nl.preproc_end) {
				fprintf(stderr, "we assumed newline would be the end of the include\n");
				fprintf(stderr, " in file: %s\n", file);
				fprintf(stderr, " include_text %d %d : %.*s\n", include_text.type, (int)include_text.text.count, (int)include_text.text.count, include_text.text.items);
				if (included) {
					fprintf(stderr, " %.*s\n", (int)found_fp.count, found_fp.items);
				}
			}

			while (cursor < lexer.content_length && &(lexer.content[cursor]) != nl.text.items) {
				if (!included) {
					da_append(&a->output, lexer.content[cursor]);
				}
				cursor++;
			}
		} else if (lexer.preprocessor_mode && token.type == TOKEN_SYMBOL && strncmp(token.text.items, "#pragma", 7)==0) {
			bool next_is_once = (strncmp(lexer_next(&lexer).text.items, "once", 4) == 0);
			if (next_is_once) {
				lexer_skip_until_new_line(&lexer);
				Token nl = lexer_next(&lexer);
				assert(nl.preproc_end && "we assumed newline would be the end of the pragma once");
				while (cursor < lexer.content_length && &(lexer.content[cursor]) != nl.text.items) {;
					cursor++;
				}
			}
		} else {
			// any other token
			if (!pack_tight || token.type != TOKEN_NEWLINE) {
				da_append_token(&a->output, token);
			}
			cursor += token.text.count;
		}

		Token prev = token;
		token = lexer_next(&lexer);

		if (pack_tight) {
			if (prev.type == TOKEN_COMMENT || prev.preproc_end) {
				da_append(&a->output, '\n');
			}
			if (token.type == TOKEN_SYMBOL || token.type == TOKEN_INTEGER) {
				if (prev.type == TOKEN_SYMBOL) {
					da_append(&a->output, ' ');
				}
			}
		}
	}

	free(source.items);
	return true;
}

void print_usage() {
	printf("usage: amalgamator <source_files> -I <include_dirs> -o <output_file>\n");
}

int main(int argc, char *argv[]) {
	int result = 0;

	if (argc < 5) {
		print_usage();
		return 1;
	}

	Amalgamator a = {
		.output = {0},
		.included_files = {0},
		.source_files_count = 0,
		.include_dirs_count = 0,
		.output_file = NULL
	};

	int opt;
	while ((opt = getopt(argc, argv, "I:o:")) != -1) {
		switch (opt) {
			case 'I':
				a.include_dirs[a.include_dirs_count++] = optarg;
				break;
			case 'o':
				a.output_file = optarg;
				break;
			default:
				print_usage();
				result = 1; goto defer;
		}
	}

	for (int i = optind; i < argc; i++) {
		a.source_files[a.source_files_count++] = argv[i];
	}

	if (a.source_files_count == 0 || a.output_file == NULL) {
		print_usage();
		result = 1; goto defer;
	}


	StringBuilder sb_source_file = {0};
	for (size_t i = 0; i < a.source_files_count; ++i) {
		sb_source_file.count = 0;
		if (find_file_path(&a, a.source_files[i], &sb_source_file)) {
			if (!included_files_contains(&a.included_files, sb_source_file.items)) {
				included_files_add(&a.included_files, sb_source_file.items);
				if (!process_file(&a, sb_source_file.items)) {
					printf("failed to process file %s\n", a.source_files[i]);
					result = 1; goto defer;
				}
			}
		} else {
			fprintf(stderr, "file not found: %s", a.source_files[i]);
			result = 1; goto defer;
		}
	}

	if (!write_to_file(a.output_file, &a.output)) {
		perror("write_to_file error");
		result = 1; goto defer;
	}

defer:
	if (result == 0) {
		printf("amalgamation completed successfully.\n");
	} else {
		printf("amalgamation failed.\n");
	}
	free(a.output.items);
	free(a.included_files.items);
	free(sb_source_file.items);
	return result;
}


