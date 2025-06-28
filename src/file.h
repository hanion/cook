#pragma once
#include "da.h"
#include <stdbool.h>
#include <sys/stat.h>

bool read_entire_file(const char *filepath_cstr, StringBuilder *sb);
bool write_to_file   (const char *filepath_cstr, StringBuilder *sb);

const char* get_filename(const char* filepath_cstr);

bool ends_with(const char* cstr, const char* w);


#ifdef _WIN32
	#define MKDIR(path) mkdir(path)
#else
	#include <unistd.h>
	#define MKDIR(path) mkdir(path, 0777)
#endif
