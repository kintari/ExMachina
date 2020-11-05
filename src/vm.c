#include "vm.h"
#include "opcode.h"
#include "trace.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static const char *OPCODE_STRINGS[256] = {
#define X(M) #M,
	MNEMONICS
#undef X
};

#define X(M) void op_ ## M(VM *vm, Frame *frame);
	MNEMONICS
#undef X

#define FETCH_TYPES \
	X(u8) \
	X(s8) \
	X(u32) \
	X(s32)
#define X(T) \
	T Fetch_ ## T(const Frame *frame) { \
		return *(const T *) &(frame)->Function->Body.Bytes[(frame)->PC+1]; \
	}
	FETCH_TYPES
#undef X

u32 CheckAddress(const VM *vm, u32 addr) {
	PANIC_IF(vm, addr >= MEMORY_SIZE);
	return addr;
}

u32 Load(const VM *vm, u32 addr) {
	return (vm)->Memory[CheckAddress(vm, addr)];
}

void Store(VM *vm, u32 addr, u32 value) {
	vm->Memory[CheckAddress(vm, addr)] = value;
}

void Push(VM *vm, u32 x) {
	Frame *frame = CURRENT_FRAME(vm);
	Store(vm, frame->SP++, x);
}

u32 Pop(VM *vm) {
	Frame *frame = CURRENT_FRAME(vm);
	PANIC_IF(vm, frame->SP == frame->BP);
	return Load(vm, --frame->SP);
}

NORETURN void VM_Panic(const VM *vm, const char *format, ...) {
	fflush(stdout);
	char buf[1024];
	va_list args;
	va_start(args, format);

	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	const Frame *frame = CURRENT_FRAME(vm);
	fputs("\n\n*** PANIC ***\n", stderr);
	fputs(buf, stderr);
	fputc('\n', stderr);

	fprintf(stderr, "    PC=%s+%04Xh BP=%04Xh SP=%04Xh\n",
		frame->Function->Name, frame->PC, frame->BP, frame->SP);

	fprintf(stderr, "    BP [ ");
	for (u32 addr = frame->BP; addr < frame->SP; addr++)
		fprintf(stderr, "%d ", vm->Memory[addr]);
	fprintf(stderr, "] SP\n");

	fprintf(stderr, "    Memory:\n    ");
	for (u32 i = 0; i < 20; i++)
		fprintf(stderr, "%02X ", vm->Memory[i]);
	fputc('\n', stderr);
	fflush(stderr);
	abort();
}

u32 VM_GetArg(const VM *vm, u32 index, u32 *arg) {
	const Frame *frame = CURRENT_FRAME(vm);
	PANIC_IF(vm, index >= frame->Function->NumArgs);
	return Load(vm, ARG_ADDRESS(frame, index));
}

void RunNativeMethod(VM *vm, Frame *frame) {
	frame->Function->Native(vm);
	vm->CallStack.Depth -= 1;
	if (frame->SP > frame->BP) {
		u32 value = Load(vm, frame->SP - 1);
		Push(vm, value);
	}
}

void StepBytecode(VM *vm) {
	Frame *frame = CURRENT_FRAME(vm);
	PANIC_IF(vm, frame->PC >= frame->Function->Body.Length);

	u8 opcode = frame->Function->Body.Bytes[frame->PC];
	u32 depth = frame->SP - frame->BP;

	const char *mnemonic = OPCODE_STRINGS[opcode];
	if (mnemonic == NULL)
		mnemonic = "???";

	TRACE(
		"%10s+%04Xh %4d %4.02Xh   %s ",
		frame->Function->Name, frame->PC, depth, opcode, mnemonic);

	u32 pc = frame->PC;

	switch (opcode) {
#define X(M) \
		case M: { \
			op_ ## M(vm, frame); \
			break; \
		}
		MNEMONICS
#undef X
		default: {
			VM_Panic(vm, "unrecognized opcode: %02Xh", opcode);
		}
	}

	TRACE("\n");
	if (opcode == CALL || opcode == RET)
		TRACE("\n");

	if ((vm->Flags & VMFLAG_HALT) == 0)
		if (CURRENT_FRAME(vm) == frame && frame->PC == pc)
			VM_Panic(vm, "remember to advance PC");
}

void VM_Step(VM *vm) {
	PANIC_IF(vm, vm->CallStack.Depth == 0);
	PANIC_IF(vm, vm->Flags & (VMFLAG_HALT | VMFLAG_BREAKPOINT));
	Frame *frame = CURRENT_FRAME(vm);
	if (frame->Function->Flags & FF_NATIVE) {
			RunNativeMethod(vm, frame);
	}
	else {
		StepBytecode(vm);
	}
}

void VM_Run(VM *vm) {
	while ((vm->Flags & (VMFLAG_HALT | VMFLAG_BREAKPOINT)) == 0)
		VM_Step(vm);
}
