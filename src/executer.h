#pragma once
#include "build_command.h"

void execute_build_command(BuildCommand* bc);

#include <stdint.h>
#include <stdbool.h>

uint64_t get_modification_time_sv(StringView path);
uint64_t get_modification_time(const char *path_cstr);


