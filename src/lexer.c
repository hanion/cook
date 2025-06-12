#include "lexer.h"
#include <assert.h>
#include <ctype.h>


Lexer lexer_new(StringBuilder sb) {
	Lexer lexer = {
		.content = sb.items,
		.content_length = sb.count,
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
		token_print(t, 0);
		t = lexer_next_token(&copy);
	}
}
