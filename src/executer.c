#include "executer.h"
#include "file.h"
#include "build_command.h"
#include "target.h"
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

Executer executer_new(Arena* arena) {
	Executer e = {0};
	e.arena = arena;
	return e;
}

void execute_build_command(Executer* e, BuildCommand* bc, bool execute_lines) {
	if (!bc || !bc->dirty) {
		return;
	}

	// create output dir
	if (execute_lines) {
		StringBuilder sb = {0};
		da_append_many(&sb, bc->output_dir.items, bc->output_dir.count);
		da_append(&sb,'\0');
		if (access(sb.items, F_OK) != 0) {
			MKDIR(sb.items);
		}
		free(sb.items);
	}

	for (size_t i = 0; i < bc->children.count; ++i) {
		execute_build_command(e, bc->children.items[i], execute_lines);
	}

	bool already_executed = false;
	for (size_t b = 0; b < e->executed.count; ++b) {
		if (build_command_is_same(bc, e->executed.items[b])) already_executed = true;
	}
	if (already_executed) return;
	da_append(&e->executed, bc);

	
	for (size_t i = 0; i < bc->targets.count; ++i) {
		Target* t = &bc->targets.items[i];
		if (!t->dirty) continue;

		bool already_built = false;
		for (size_t b = 0; b < e->built.count; ++b) {
			if (target_is_same(t, e->built.items[b])) already_built = true;
		}
		if (already_built) continue;
		da_append(&e->built, t);

		StringBuilder sb = target_generate_cmdline_cstr(e->arena, bc, t);

		if (execute_lines) {
			printf("$ %.*s\n", (int)sb.count, sb.items);
			int ret = execute_line(sb.items);
			if (ret != 0) break;
		} else {
			printf("%.*s\n", (int)sb.count, sb.items);
		}
	}
}

void executer_dry_run(Executer* e, BuildCommand* root) {
	e->built.count = 0;
	e->executed.count = 0;
	execute_build_command(e, root, false);
}

void executer_execute(Executer* e, BuildCommand* root) {
	e->built.count = 0;
	e->executed.count = 0;
	execute_build_command(e, root, true);
}





#ifdef _WIN32
	#include <windows.h>
	#include <sys/types.h>
	#include <sys/stat.h>
#else
	#include <sys/stat.h>
	#include <time.h>
#endif

uint64_t get_modification_time(const char *path_cstr) {
#ifdef _WIN32
	WIN32_FILE_ATTRIBUTE_DATA attr;
	if (!GetFileAttributesExA(path_cstr, GetFileExInfoStandard, &attr)) {
		return 0;
	}

	FILETIME ft = attr.ftLastWriteTime;
	ULARGE_INTEGER ull;
	ull.LowPart  = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

	return (ull.QuadPart - 116444736000000000ULL) / 10000000ULL;
#else
	struct stat st;
	if (stat(path_cstr, &st) != 0) {
		return 0;
	}
	return (uint64_t)st.st_mtime;
#endif
}

uint64_t get_modification_time_sv(StringView path) {
	char buf[512];
	assert(path.count < sizeof(buf));
	memcpy(buf, path.items, path.count);
	buf[path.count] = '\0';
	return get_modification_time(buf);
}
