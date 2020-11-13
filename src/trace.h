#pragma once

#include <stdio.h>

#define TRACE output
#define ERROR output

void output(const char *format, ...);

#define ASSERT(cond) assert(cond)
#define ASSERT_IF_NULL(expr) { if ((expr) == NULL) assert(#expr && 0); }

#ifdef _DEBUG
int VerifyFail(const char *msg);
#define VERIFY(expr) ((expr) || VerifyFail(#expr))
#else
#define VERIFY(expr) (expr)
#endif

