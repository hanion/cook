#include "parser.h"
#include "da.h"
#include "expression.h"
#include "lexer.h"
#include "statement.h"
#include "token.h"
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
		 error_cstr, token_name_cstr(token), (int)token.length, token.text);
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
			//Token name = parser_consume(p, TOKEN_IDENTIFIER, "Expected method name after '.'.");

			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_CHAIN;
			e->chain.expr = expr;
			e->chain.right = right;
			expr = e;

		} else if (parser_match(p, TOKEN_COLON) && parser_match(p, TOKEN_COLON)) {
			return parse_call(p);
// 		} else if (parser_check(p, TOKEN_OPEN_BRACKET)) {
// 			Token name = p->previous;
// 			parser_match(p, TOKEN_OPEN_BRACKET);
// 			Expression* key = parse_expression(p);
// 			parser_consume(p, TOKEN_CLOSE_BRACKET, "Expected ']' after subscript expression.");
// 
// 			Expression* e = parser_arena_alloc_expression(p);
// 			e->type = EXPR_SUBSCRIPT;
// 			e->subscript.expr = expr;
// 			e->subscript.key = key;
// 			e->subscript.name = name;
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
			e->variable.name  = p->previous;
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
			e->literal_int.value = parse_literal_int(p->previous.text, p->previous.length);
			return e;
		}
		case TOKEN_FLOAT_LITERAL: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_FLOAT;
			e->literal_float.value = parse_literal_float(p->previous.text, p->previous.length);
			return e;
		}
		case TOKEN_STRING_LITERAL: {
			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_STRING;
			e->literal_string.value  = p->previous.text;
			e->literal_string.length = p->previous.length;
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

			const char* start = t.text;
			while (!parser_check(p, TOKEN_CLOSE_PAREN)
				&& !parser_check(p, TOKEN_COMMA)
				&& !parser_check(p, TOKEN_DOLLAR)) {
				t = parser_advance(p);
			}
			const char* end = t.text + t.length;

			Expression* e = parser_arena_alloc_expression(p);
			e->type = EXPR_LITERAL_STRING;
			e->literal_string.value = start;
			e->literal_string.length = end - start;
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
		// TODO: use arena instead of malloc
		da_append(&statement_list, s);
	}

	return statement_list;
}

Statement* parse_declaration(Parser* p) {
// 	if (parser_check(p, TOKEN_IDENTIFIER) && parser_check_next(p, TOKEN_COLON)) {
// 		return parse_typed_declaration(p);
// 	}
	return parse_statement(p);
}
Statement* parse_statement(Parser* p) {
	if (parser_match(p, TOKEN_OPEN_CURLY)) {
		return parse_block_statement(p);
	}
	return parse_expression_statement(p);
}

Statement* parse_block_statement(Parser* p) {
	StatementList statement_list = statement_list_new();

	while (!parser_check(p, TOKEN_CLOSE_CURLY) && !parser_is_at_end(p)) {
		Statement* s = parse_declaration(p);
		// TODO: use arena instead of malloc
		da_append(&statement_list, s);
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
	parser_match(p, TOKEN_SEMICOLON);
	Statement* s = parser_arena_alloc_statement(p);
	s->type = STATEMENT_EXPRESSION;
	s->expression.expression = expr;
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
		statement_print(s, 0);
	}
}
