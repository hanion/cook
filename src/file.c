#include "file.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


bool read_entire_file(const char *filepath_cstr, StringBuilder *sb) {
	bool result = true;

	FILE *f = fopen(filepath_cstr, "rb");
	if (f == NULL) { result = false; goto defer; }
	if (fseek(f, 0, SEEK_END) < 0) { result = false; goto defer; }

#ifndef _WIN32
	long m = ftell(f);
#else
	long long m = _ftelli64(f);
#endif

	if (m < 0) { result = false; goto defer; }
	if (fseek(f, 0, SEEK_SET) < 0) { result = false; goto defer; }

	size_t new_count = sb->count + m;
	if (new_count > sb->capacity) {
		sb->items = realloc(sb->items, new_count);
		assert(sb->items != NULL);
		sb->capacity = new_count;
	}

	fread(sb->items + sb->count, m, 1, f);
	if (ferror(f)) { result = false; goto defer; }
	sb->count = new_count;

defer:
	if (!result) { printf("Could not read file %s: %s\n", filepath_cstr, strerror(errno)); }
	if (f) { fclose(f); }
	return result;
}

bool write_to_file(const char *filepath_cstr, StringBuilder *sb) {
	FILE *f = fopen(filepath_cstr, "wb");
	if (f == NULL) {
		printf("Could not open file for writing: %s\n", strerror(errno));
		return false;
	}

	size_t written = fwrite(sb->items, 1, sb->count, f);
	if (written != sb->count) {
		printf("Error writing to file: %s\n", strerror(errno));
		fclose(f);
		return false;
	}

	fclose(f);
	return true;
}


const char* get_filename(const char* filepath_cstr) {
	const char* last_slash = strrchr(filepath_cstr, '/');
	if (!last_slash) {
		return filepath_cstr;
	}
	return last_slash + 1;
}

bool ends_with(const char* cstr, const char* w) {
	size_t len_str = strlen(cstr);
	size_t len_w = strlen(w);
	if (len_w > len_str) {
		return false;
	}
	return strcmp(cstr + len_str - len_w, w) == 0;
}
