#include "trace.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>

extern void OutputDebugStringA(const char *);
extern void DebugBreak();

void output(const char *format, ...) {
	va_list args;
	va_start(args, format);
	size_t count = _vscprintf(format, args);
	char *buf = _malloca(count + 2);
	if (VERIFY(buf)) {
		_vsnprintf_s(buf, count + 1, count, format, args);
		buf[count] = '\n';
		buf[count + 1] = '\0';
		fwrite(buf, 1, count, stdout);
		fputc('\n', stdout);
		OutputDebugStringA(buf);
	}
	va_end(args);
}

int VerifyFail(const char *msg) {
	output(msg);
	DebugBreak();
	return 0;
}