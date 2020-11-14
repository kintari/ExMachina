
#include "io.h"

#include <stdio.h>
#include <stdlib.h>

#include "types.h"

extern void OutputDebugStringA(const char *str);

#define LOCK(x)
#define UNLOCK(x)

#define MUTEX_TYPE int
static MUTEX_TYPE mutex;

#define PRINT_BUF_SIZE 1024

char PrintBuf[PRINT_BUF_SIZE];
u32 WritePos;


static void Flush() {
	if (WritePos > 0) {
		fwrite(PrintBuf, 1, WritePos, stdout);
		OutputDebugStringA(PrintBuf);
		memset(PrintBuf, 0, PRINT_BUF_SIZE);
		WritePos = 0;
	}
}

static void PrintChar(int ch) {
	LOCK(mutex);
	if (WritePos + 1 >= PRINT_BUF_SIZE)
		Flush();
	PrintBuf[WritePos++] = (char) ch;
	if (ch == '\n')
		Flush();
	UNLOCK(mutex);
}

void PrintUint(const Value *value) {
	LOCK(mutex);
	int nc = _scprintf("%zu", value->Uint);
	if (WritePos + nc + 1 >= PRINT_BUF_SIZE)
		Flush();
	if (nc > 0) {
		nc = snprintf(&PrintBuf[WritePos], PRINT_BUF_SIZE - WritePos, "%zu", value->Uint);
		if (nc > 0)
			WritePos += nc;
	}
	UNLOCK(mutex);
}

void io_println_OpInvoke(AstEvalVisitor *v, void *unused) {
	LOCK(mutex);
	size_t offset = 0;
	Value operand;
	int status = 0;
	int count = NumOperands(v);
	for (int i = 0; i < count && GetOperand(v, i, &operand, &status); i++) {
		switch (operand.Type) {
			case Value_Uint: {
				PrintUint(&operand);
				break;
			}
		}
		if (i + 1 < count) {
			PrintChar(' ');
		}
	}
	PrintChar('\n');
	UNLOCK(mutex);
}
