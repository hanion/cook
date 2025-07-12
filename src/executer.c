#include "executer.h"
#include "file.h"
#include "build_command.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define CMD_LINE_MAX 2000

static inline int execute_line(const char* line) {
#ifdef _WIN32
		char full_command[CMD_LINE_MAX + 16];
		snprintf(full_command, sizeof(full_command), "cmd /C \"%s\"", line);
		return system(full_command);
#else
	return system(line);
#endif
}


void execute_build_command(BuildCommand* bc) {
	if (!bc || !bc->dirty) {
		return;
	}

	// create output dir
	{
		StringBuilder sb = {0};
		da_append_many(&sb, bc->output_dir.items, bc->output_dir.count);
		da_append(&sb,'\0');
		if (access(sb.items, F_OK) != 0) {
			MKDIR(sb.items);
		}
		free(sb.items);
	}

	FILE* script = tmpfile();
	if (!script) {
		perror("tmpfile");
		exit(1);
	}

	build_command_dump(bc, script, 0);

	rewind(script);
	char line[CMD_LINE_MAX];
	while (fgets(line, sizeof(line), script)) {
		size_t len = strlen(line);
		while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
			line[--len] = '\0';
		}
		printf("$ %.*s\n", (int)len, line);
		int ret = execute_line(line);
		if (ret != 0) {
			break;
		}
	}

	fclose(script);
}
