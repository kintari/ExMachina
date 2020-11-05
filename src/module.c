
#include "module.h"
#include "opcode.h"
#include "vm.h"
#include "trace.h"

// function fib(x) {
//	if (x == 0) return 0;
//  if (x == 1) return 0;
//  return (x-2 + (x-1);)
// }

#define FIB_INDEX 2
#define PRINTLN_INDEX 3

u8 FibBody[] = {
	DUP,
	PUSH, $(1), // Check to see if it is an end case
	LTE,
	BZ, 6,
	PUSH, $(1),
	RET,
	DUP, // Start of recursive case ... fib(x-2)
	PUSH, $(-2),
	ADD,
	CALL, $(FIB_INDEX), 
	XCHG, // This block is fib(x-1)
	PUSH, $(-1),
	ADD,
	CALL, $(FIB_INDEX),
	ADD,
	RET
};

u8 MainBody[] = {
	PUSH, $(0), // loop variable
	DUP,
	CALL, $(FIB_INDEX),
	CALL, $(PRINTLN_INDEX), // index of print
	PUSH, $(1), // increment loop variable
	ADD,
	DUP,
	PUSH, $(10), // test against limit
	LT,
	BNZ, -26,
	RET
};

u8 GlobalBody[] = {
	CALL, $(1),
	HALT
};

void Println(VM *vm) {
	Frame *frame = CURRENT_FRAME(vm);
	PANIC_IF(vm, frame->Function == 0);
	u32 nargs = frame->Function->NumArgs;
	u32 arg;
	trace("[stdout] ");
	for (u32 i = 0; i < nargs; i++) {
		u32 arg = vm->Memory[ARG_ADDRESS(frame, i)];
		trace("%d", arg);
	}
	trace("\n");
	CURRENT_FRAME(vm)->SP -= nargs;
}

Function myModule_Functions[] = (Function[]){
	(Function) {
		.Name = "$global",
		.NumArgs = 0,
		.Body = {
			GlobalBody,
			sizeof(GlobalBody)
		},
		.Flags = 0
	},
	(Function) {
		.Name = "main",
		.NumArgs = 0,
		.Body = {
			MainBody, 
			sizeof(MainBody)
		},
		.Flags = 0
	},
	(Function) {
		.Name = "fib",
		.NumArgs = 1,
		.Body = {
			FibBody, 
			sizeof(FibBody)
		},
		.Flags = 0
	},
	(Function) {
		.Name = "println",
		.NumArgs = 1,
		.Native = Println, 
		.Flags = FF_NATIVE
	}
};

Module myModule = {
	.Functions = myModule_Functions,
	.NumFunctions = countof(myModule_Functions)
};

const Module *LoadModule() {
	return &myModule;
}