#pragma once

#include "types.h"
#include "function.h"
#include "config.h"
#include "module.h"

DECLARE_TYPE(Frame);
DECLARE_TYPE(VM);
DECLARE_TYPE(Module);

// A frame holds data pertaining to a particular function activation
struct Frame {
	u32 PC, BP, SP;
	const Function *Function;
};

#define VMFLAG_HALT 			0x01
#define VMFLAG_BREAKPOINT 0x02

struct VM {
	struct {
		Frame Frames[MAX_FRAMES];
		u32 Depth;
	} CallStack;
	const Module *Module;
	u32 Memory[MEMORY_SIZE];
	u32 Flags;
};

#define CURRENT_FRAME(vm) (&(vm)->CallStack.Frames[(vm)->CallStack.Depth - 1])
#define $(x) (x) & 0xff, (x) >> 8 & 0xff, (x) >> 16 & 0xff, (x) >> 24 & 0xff
#define PANIC_IF(vm, cond) { if (cond) VM_Panic(vm, #cond); }
#define ARG_ADDRESS(f, i) ((f)->BP + i)

void VM_Run(VM *vm);

NORETURN void VM_Panic(const VM *vm, const char *reason, ...);
