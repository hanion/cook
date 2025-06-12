#pragma once
#include "da.h"
#include "token.h"
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
	Expression* expr;
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

