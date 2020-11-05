#include "trace.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

void trace(const char *format, ...) {
	char buf[1024];
	va_list args;
	va_start(args, format);
	size_t count = vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	fwrite(buf, 1, count, stdout);
}