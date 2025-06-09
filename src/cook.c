/*
 * cook - an imperative build tool
 *
 * inspired by and includes code from:
 *     nob - v1.20.6 - Public Domain - https://github.com/tsoding/nob.h
 *     by Tsoding - https://github.com/tsoding
 *
 * author: haion.dev - github.com/hanion
 */

#include "file.h"
#include "parser.h"

// #define shift(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)

int main(int argc, char** argv) {
	int result = 0;
// 	while (argc > 0) {
// 		printf("%s\n", shift(argv, argc));
// 	}

	StringBuilder source = sb_new();

	char* const filepath = "./Cookfile";
	if (false && !read_entire_file(filepath, &source)) { result = 1; goto defer; }

	const char * const program = "build(foo.c, bar.c , dir/asd.abcd,   hoh.hoo)\ninput(baz.c $varr)\n\t.cflags(-Wall, -Werror)";
	da_append_many(&source, program, strlen(program));
	
	Lexer lexer = lexer_new(source);

// 	Lexer copy = lexer;
// 	Token t = lexer_next_token(&copy);
// 	while (t.type != TOKEN_END) {
// 		printf("%s %.*s\n", token_name_cstr(t), (int)t.length, t.text);
// 		t = lexer_next_token(&copy);
// 	}
// 	printf("======\n");

	Parser parser = parser_new(&lexer);

// 	Expression* e = parse_expression(&parser);
// 	while (e != NULL) {
// 		expression_print(e, 0);
// 		e = parse_expression(&parser);
// 	}

	StatementList sl = parser_parse_all(&parser);
	for (size_t i = 0; i < sl.count; ++i) {
		Statement* s = sl.items[i];
		statement_print(s, 0);
	}

defer:
	if (result != 0) {
		printf("could not intertpret file %s\n", filepath);
	}
	sb_free(&source);
	return result;
}
