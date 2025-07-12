#pragma once

#include "da.h"
#include <stdbool.h>

typedef struct CookOptions {
	StringView source;
	int verbose;
	bool dry_run;
	bool build_all;
} CookOptions;

static inline CookOptions cook_options_default(void) {
	return (CookOptions){
		.verbose = 0,
		.dry_run = false,
		.build_all = false,
	};
}

int cook(CookOptions op);

