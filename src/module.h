#pragma once

#include "types.h"
#include "function.h"

typedef struct Module {
	const Function *Functions;
	u32 NumFunctions;
} Module;

const Module *LoadModule();