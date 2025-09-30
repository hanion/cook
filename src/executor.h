#pragma once
#include "build_command.h"
#include "evaluator.h"
#include "runner.h"


typedef struct {
	Arena arena;
	int verbose;
	bool had_error;
	bool dry_run;
	BuildCommand* root_build_command;
	Environment* current_environment;
	Evaluator evaluator;
	Runner runner;
} Executor;

Executor executor_new(BuildCommand* bc, bool dry_run);
void executor_run(Executor*);
