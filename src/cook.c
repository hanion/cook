#include "cook.h"
#include "arena.h"
#include "build_command.h"
#include "planner.h"
#include "lexer.h"
#include "parser.h"
#include "executor.h"


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

	Planner planner = planner_new(root_statement);
	BuildCommand* root_build_command = planner_construct_build_command(&planner);

	if (op.dry_run || op.build_all) {
		build_command_mark_all_children_dirty(root_build_command, true);
	}

	if (op.verbose > 0) {
		printf("[cook] build command pretty:\n");
		build_command_print(root_build_command, 0);

		if (op.dry_run) {
			printf("[cook] build command dump:\n");
		}
	}

	Executor executor = executor_new(root_build_command, op.dry_run);
	executor_run(&executor);

	arena_free(&executor.arena);
	arena_free(&parser.arena);
	arena_free(&planner.arena);
	return 0;
}

