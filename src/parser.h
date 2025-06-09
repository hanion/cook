#pragma once
#include "arena.h"
#include "lexer.h"
#include "expression.h"
#include "statement.h"

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
