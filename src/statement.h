#pragma once
#include "expression.h"

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


StatementList statement_list_new(void);

const char* statement_name_cstr(StatementType st);

void statement_print(Statement* s, int indent);
