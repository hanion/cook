#pragma once
#include "da.h"
#include "token.h"

typedef struct {
	const char* content;
	size_t content_length;
	size_t cursor;
	size_t line;
	size_t line_start;
} Lexer;

Lexer lexer_new(StringBuilder sb);

char lexer_advance         (Lexer* l);
bool lexer_check           (Lexer* l, char c);
bool lexer_match           (Lexer* l, char c);
bool lexer_check_next      (Lexer* l, char c);
void lexer_trim_left       (Lexer* l);
void lexer_skip_to_new_line(Lexer* l);

Token lexer_next_token(Lexer* l);
Token lexer_peek_next (Lexer* l);
