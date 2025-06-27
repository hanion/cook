#include "cook.h"
#include "interpreter.h"


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

