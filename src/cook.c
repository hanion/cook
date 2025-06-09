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
	//lexer_dump(&lexer);
	Parser parser = parser_new(&lexer);
	parser_dump(&parser);

defer:
	if (result != 0) {
		printf("could not intertpret file %s\n", filepath);
	}
	sb_free(&source);
	return result;
}
