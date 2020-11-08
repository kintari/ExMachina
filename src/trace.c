#include "trace.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

extern void OutputDebugStringA(const char *);

void trace(const char *format, ...) {
	va_list args;
	va_start(args, format);
	size_t count = _vscprintf(format, args);
	char *buf = _malloca(count + 2);
	if (buf) _vsnprintf_s(buf, count + 1, count, format, args);
	buf[count] = '\n';
	buf[count + 1] = '\0';
	va_end(args);
	fwrite(buf, 1, count, stdout);
	OutputDebugStringA(buf);
}