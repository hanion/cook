/*
 * cook - an imperative build tool
 *
 * includes chunks of code from:
 *     nob - v1.20.6 - Public Domain - https://github.com/tsoding/nob.h
 *     by Tsoding - https://github.com/tsoding
 *
 * author: haion.dev - github.com/hanion
 */

#include "da.h"
#include "file.h"
#include "interpreter.h"
#include "parser.h"

typedef struct CookOptions {
	StringView source;
	bool dry_run;
} CookOptions;

CookOptions cook_options_default() {
	return (CookOptions){
		.dry_run = true,
	};
}


int cook(CookOptions op) {
	Lexer lexer = lexer_new(op.source);
	//lexer_dump(&lexer);
	Parser parser = parser_new(&lexer);
	//parser_dump(&parser);
	Interpreter interpreter = interpreter_new(&parser);

	if (op.dry_run) {
		interpreter_dry_run(&interpreter);
	} else {
		interpreter_interpret(&interpreter);
	}

	return 0;
}


void print_usage(const char* pname) {
	fprintf(stderr,
		"usage: %s [options]\n"
		"\n"
		"options:\n"
		"  -h, --help      show this help message\n"
		"  -f <file>       use specified cookfile\n"
		"  --dry-run       show the commands that would be run, but don't execute them\n",
		pname
	);
}

#define shift(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)

int main(int argc, char** argv) {
	CookOptions op = cook_options_default();

	const char* pname = shift(argv, argc);
	const char* filepath = NULL;

	while (argc > 0) {
		const char* arg = shift(argv, argc);

		if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			print_usage(pname);
			return 0;
		} else if (strcmp(arg, "-f") == 0) {
			if (argc == 0) {
				fprintf(stderr, "[ERROR] expected a filepath after -f\n");
				print_usage(pname);
				return 1;
			}
			filepath = shift(argv, argc);
		} else if (strcmp(arg, "--dry-run") == 0) {
			op.dry_run = false;
		} else {
			fprintf(stderr, "[ERROR] unrecognized argument: %s\n", arg);
			print_usage(pname);
			return 1;
		}
	}

	StringBuilder source = {0};

	if (filepath) {
		StringBuilder file = {0};
		da_append_many(&file, filepath, strlen(filepath));
		da_append_many(&file, "/Cookfile", 9);
		da_append(&file, 0);

		if (!read_entire_file(file.items, &source)) {
			return 1;
		}
		op.source = sv_from_sb(source);
		sb_free(&file);
	} else {
		const char * const program = "build(game)\
			.input(main.cpp, input.cpp, window.cpp, renderer.cpp, imgui.cpp)\
			.compiler(g++).cflags(-Wall, -Wextra, -Werror, -g)\
			.include_dir(include, imgui, libs/glfw/include, libs/glew/include, libs/glm)\
			.library_dir(libs/glfw/lib, libs/glew/lib).link(glfw, GLEW, GL, dl, pthread)\
			.source_dir(src).output_dir(build)";
		da_append_many(&source, program, strlen(program));
		op.source = sv_from_sb(source);
	}

	bool result = cook(op);

	sb_free(&source);
	return result;
}
