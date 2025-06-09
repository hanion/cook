#include "token.h"
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
	#define KW(k,l) (token.length == l && 0 == strncmp(k, token.text, l))
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
	if(token.length != strlen(cstr)) {
		return false;
	}
	return memcmp(token.text, cstr, token.length) == 0;
}

inline static void print_indent(int indent) {
	for (int i = 0; i < indent; ++i) {
		putchar(' ');
	}
}
void token_print(Token t, int indent) {
	print_indent(indent);
	printf("token: %zu:%zu %s '%.*s'\n", t.line, t.column, token_name_cstr(t), (int)t.length, t.text);
}

