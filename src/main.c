/*
 * cook - better make
 *
 * includes chunks of code from:
 *     nob - v1.20.6 - Public Domain - https://github.com/tsoding/nob.h
 *     by Tsoding - https://github.com/tsoding
 *
 * author: haion.dev - github.com/hanion
 */

#include "cook.h"
#include "file.h"
#include <stdio.h>
#include <unistd.h>

void print_usage(const char* pname) {
	fprintf(stderr,
		"cook - better make\n"
		"usage: %s [options]\n"
		"\n"
		"options:\n"
		"  -h, --help      show this help message\n"
		"  -f <file>       use specified cookfile\n"
		"  --verbose       verbose printing\n"
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
			op.dry_run = true;
		} else if (strncmp(arg, "--verbose=", 10) == 0) {
			op.verbose = arg[10] - '0';
		} else if (strcmp(arg, "--verbose") == 0) {
			op.verbose = 1;
		} else {
			fprintf(stderr, "[ERROR] unrecognized argument: %s\n", arg);
			print_usage(pname);
			return 1;
		}
	}

	StringBuilder source = {0};

	if (filepath) {
		if (!read_entire_file(filepath, &source)) {
			return 1;
		}
		op.source = sv_from_sb(source);
	} else if (access("./Cookfile", F_OK) == 0 && read_entire_file("./Cookfile", &source)) {
		op.source = sv_from_sb(source);
	} else {
		print_usage(pname);
		return 1;
	}

	bool result = cook(op);

	sb_free(&source);
	return result;
}
