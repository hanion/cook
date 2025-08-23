#include "cook.h"
#include "arena.h"
#include "build_command.h"
#include "constructor.h"
#include "executer.h"
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"


int cook(CookOptions op) {
	Lexer lexer = lexer_new(op.source);

	if (op.verbose > 3) {
		printf("[file] dump:\n");
		printf("%.*s\n", (int)op.source.count, op.source.items);
	}
	if (op.verbose > 2) {
		printf("[lexer] dump:\n");
		lexer_dump(&lexer);
	}

	Parser parser = parser_new(&lexer);
	Statement* root_statement = parser_parse_all(&parser);

	if (op.verbose > 1) {
		printf("[parser] dump:\n");
		statement_print(root_statement, 1);
	}

	Constructor constructor = constructor_new(root_statement);
	BuildCommand* root_build_command = constructor_construct_build_command(&constructor);

	if (op.build_all) {
		build_command_mark_all_children_dirty(root_build_command, true);
	}

	if (op.verbose > 0) {
		printf("[cook] build command pretty:\n");
		build_command_print(root_build_command, 0);
	}

	Interpreter interpreter = interpreter_new(root_build_command);
	interpreter_interpret(&interpreter);

	Executer e = executer_new(&interpreter.arena);

	if (op.dry_run) {
		if (op.verbose > 0) {
			printf("[cook] build command dump:\n");
		}
		build_command_mark_all_children_dirty(root_build_command, true);
		executer_dry_run(&e, root_build_command);
	} else {
		executer_execute(&e, root_build_command);
	}

	arena_free(&interpreter.arena);
	arena_free(&parser.arena);
	arena_free(&constructor.arena);
	free(e.executed.items);
	free(e.built.items);
	return 0;
}

