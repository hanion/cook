#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include "da.h"
#include "file.h"

#define TEST_COUNT 2
static const char* tests[TEST_COUNT] = {
	"hello_world",
	"complex"
};


FILE * popen(const char*, const char*);
int pclose(FILE*);


bool compare_expected_cmd(const char* expected_cmd_path_cstr, StringBuilder* output) {
	StringBuilder file = {0};
	if (!read_entire_file(expected_cmd_path_cstr, &file)) {
		return false;
	}

	// strip trailing whitespace
	while (file.count > 0 && (
		file.items[file.count - 1] == '\n' ||
		file.items[file.count - 1] == ' '  ||
		file.items[file.count - 1] == '\r' )) {
		file.count--;
	}
	while (output->count > 0 && (
		output->items[output->count - 1] == '\n' ||
		output->items[output->count - 1] == ' '  ||
		output->items[output->count - 1] == '\r' )) {
		output->count--;
	}


	bool result = (strncmp(file.items, output->items, output->count) == 0);

	if (!result) {
		printf("failed\n");
		printf("\nfile (%zu lines):\n%.*s", file.count, (int)file.count, file.items);
		printf("\noutput (%zu lines):\n%.*s", output->count, (int)output->count, output->items);
		printf("\n");
	}
	sb_free(&file);
	return result;
}

bool run_test(const char* cmd_cstr, StringBuilder* cmd) {
	FILE *p = popen(cmd_cstr, "r");
	if (p == NULL) {
		perror("popen failed");
		return false;
	}
	char buf[256];
	size_t n;

	while ((n = fread(buf, 1, sizeof(buf), p)) > 0) {
		da_append_many(cmd, buf, n);
	}

	pclose(p);
	return true;
}

int main(void) {
	const char* build_path = "build/";
	const char* tests_path = "tests/";
	const char* build_cmd_f = "cook --dry-run -f ";
	const char* expected_cmd_path = "/expected_cmd";

	StringBuilder test_cmd = {0};
	StringBuilder output_cmd = {0};
	StringBuilder expected_cmd = {0};

	size_t failed_count = 0;

	for (size_t i = 0; i < TEST_COUNT; ++i) {
		test_cmd.count = 0;
		output_cmd.count = 0;
		expected_cmd.count = 0;

		da_append_many(&test_cmd, build_path, strlen(build_path));
		da_append_many(&test_cmd, build_cmd_f, strlen(build_cmd_f));
		da_append_many(&test_cmd, tests_path, strlen(tests_path));
		da_append_many(&test_cmd, tests[i], strlen(tests[i]));
		da_append(&test_cmd, 0);

		da_append_many(&expected_cmd, tests_path, strlen(tests_path));
		da_append_many(&expected_cmd, tests[i], strlen(tests[i]));
		da_append_many(&expected_cmd, expected_cmd_path, strlen(expected_cmd_path));
		da_append(&expected_cmd, 0);

		printf("* test[%zu](%s): ", i, tests[i]);

		bool run_test_result = run_test(test_cmd.items, &output_cmd);

		if (!run_test_result) {
			printf("failed to run\n");
			failed_count++;
			continue;
		}

		if (compare_expected_cmd(expected_cmd.items, &output_cmd)) {
			printf("passed\n");
		} else {
			failed_count++;
			// printing done in compare_expected_cmd
		}
	}

	if (failed_count == 0) {
		fprintf(stdout, "[SUCCESS][tester] passed %d tests\n", TEST_COUNT);
	} else {
		fprintf(stderr, "[ERROR][tester] failed %zu tests\n", failed_count);
	}

	sb_free(&test_cmd);
	sb_free(&output_cmd);
	sb_free(&expected_cmd);
	return 0;
}
