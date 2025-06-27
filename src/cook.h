#pragma once

#include "da.h"
#include <stdbool.h>

typedef struct CookOptions {
	StringView source;
	bool dry_run;
	int verbose;
} CookOptions;

static inline CookOptions cook_options_default() {
	return (CookOptions){
		.dry_run = true,
		.verbose = 0,
	};
}

int cook(CookOptions op);

