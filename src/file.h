#pragma once
#include "da.h"
#include <stdbool.h>

bool read_entire_file(const char *filepath_cstr, StringBuilder *sb);
bool write_to_file   (const char *filepath_cstr, StringBuilder *sb);

const char* get_filename(const char* filepath_cstr);

bool ends_with(const char* cstr, const char* w);

