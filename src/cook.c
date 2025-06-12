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
#include "interpreter.h"
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

	const char * const program = "build(game).input(main.cpp, input.cpp, window.cpp, renderer.cpp, imgui.cpp).compiler(g++).cflags(-Wall, -Wextra, -Werror, -g).include_dir(include, imgui, libs/glfw/include, libs/glew/include, libs/glm).library_dir(libs/glfw/lib, libs/glew/lib).link(glfw, GLEW, GL, dl, pthread).source_dir(src).output_dir(build)";
	da_append_many(&source, program, strlen(program));
	
	Lexer lexer = lexer_new(source);
	//lexer_dump(&lexer);
	Parser parser = parser_new(&lexer);
	//parser_dump(&parser);
	Interpreter interpreter = interpreter_new(&parser);
	interpreter_interpret(&interpreter);

defer:
	if (result != 0) {
		printf("could not intertpret file %s\n", filepath);
	}
	sb_free(&source);
	return result;
}
