#include "vm.h"
#include "opcode.h"
#include "types.h"
#include "trace.h"

#define FETCH_TYPES \
	X(u8) \
	X(s8) \
	X(u32) \
	X(s32)

#define X(T) T Fetch_ ## T(const Frame *frame);
FETCH_TYPES
#undef X
#undef FETCH_TYPES

void VM_Panic(const VM *vm, const char *format, ...);

u32 Load(const VM *vm, u32 addr);

void Store(VM *vm, u32 addr, u32 value);

void Push(VM *vm, u32 x);

u32 Pop(VM *vm);

void op_NOP(VM *vm, Frame *frame) {
	frame->PC++;
}

void op_PUSH(VM *vm, Frame *frame) {
	s32 operand = Fetch_s32(frame);
	TRACE("%d", operand);
	Push(vm, operand);
	frame->PC += 5;
}

void op_POP(VM *vm, Frame *frame) {
	u32 value = Pop(vm);
	TRACE("[=%d]", value);
	frame->PC++;
}

void op_DUP(VM *vm, Frame *frame) {
	u32 value = Load(vm, frame->SP - 1);
	TRACE("[=%d]", value);
	Store(vm, frame->SP, value);
	frame->SP++;
	frame->PC++;
}

void op_XCHG(VM *vm, Frame *frame) {
	u32 a = Load(vm, frame->SP - 1);
	u32 b = Load(vm, frame->SP - 2);
	Store(vm, frame->SP - 1, b);
	Store(vm, frame->SP - 2, a);
	frame->PC++;	
}

#define IMPLEMENT_COMPARE(mnemonic, x, oper, y) \
	void op_ ## mnemonic(VM *vm, Frame *frame) { \
		u32 res = x oper y; \
		TRACE("[=%d]", res); \
		Store(vm, frame->SP - 2, res); \
		frame->SP--; \
		frame->PC += 1; \
	}

IMPLEMENT_COMPARE(LT,  Load(vm, frame->SP - 2), <,  Load(vm, frame->SP - 1));
IMPLEMENT_COMPARE(LTE, Load(vm, frame->SP - 2), <=, Load(vm, frame->SP - 1));

#define IMPLEMENT_BRANCH(mnemonic, x, oper, y) \
	void op_ ## mnemonic(VM *vm, Frame *frame) { \
		bool branch = x oper y; \
		s8 offset = Fetch_s8(frame); \
		u32 target = frame->PC + 2 + offset; \
		if (offset < 0) { \
			TRACE("-%Xh [%04Xh,", -offset, target); \
		} \
		else { \
			TRACE("+%Xh [%04Xh,", offset, target); \
		} \
		frame->PC += 2; \
		if (branch) { \
			TRACE("y]"); \
			frame->PC = target; \
		} \
		else { \
			TRACE("n]"); \
		} \
		frame->SP--; \
	}

IMPLEMENT_BRANCH(BZ,  Load(vm, frame->SP-1), ==, 0);
IMPLEMENT_BRANCH(BNZ, Load(vm, frame->SP-1), !=, 0);
IMPLEMENT_BRANCH(BNE, Load(vm, frame->SP-2), !=, Load(vm, frame->SP-1));

#define IMPLEMENT_ARITHMETIC(mnemonic,oper) \
	void op_ ## mnemonic(VM *vm, Frame *frame) { \
		s32 a = Load(vm, frame->SP - 2); \
		s32 b = Load(vm, frame->SP - 1); \
		s32 value = a oper b; \
		TRACE("[=%d]", value); \
		Store(vm, frame->SP - 2, value); \
		frame->SP -= 1; \
		frame->PC += 1; \
	}

IMPLEMENT_ARITHMETIC(ADD, +);
IMPLEMENT_ARITHMETIC(SUB, -);
IMPLEMENT_ARITHMETIC(MUL, *);
IMPLEMENT_ARITHMETIC(DIV, /);

void op_CALL(VM *vm, Frame *frame) {
	// layout of stack right before executing call instruction:
	// [???] <- SP
	// [index of function to call]
	// [# of args to pass to function]
	// [argN]
	// [...]
	// [arg2]
	// [arg1]
	// [arg0]

	PANIC_IF(vm, vm->CallStack.Depth == MAX_FRAMES);

	// Get the callee function index and check it
	u32 fi = Fetch_u32(frame);
	if (fi >= vm->Module->NumFunctions)
		VM_Panic(vm, "Function index %d is out of bounds 0,%d", fi, vm->Module->NumFunctions);
	const Function *new_function = &vm->Module->Functions[fi];

	// Get the number of arguments and check that caller has enough
	PANIC_IF(vm, new_function->NumArgs > frame->SP - frame->BP);
	frame->SP -= new_function->NumArgs;

	// Push a new frame onto the call stack
	Frame *new_frame = &vm->CallStack.Frames[vm->CallStack.Depth++];
	new_frame->Function = &vm->Module->Functions[fi];
	new_frame->PC = 0;
	new_frame->BP = frame->SP;
	new_frame->SP = new_frame->BP + new_frame->Function->NumArgs;

	TRACE("%s ", new_function->Name);
	for (u32 i = 0; i < new_frame->Function->NumArgs; i++)
		TRACE("%d", Load(vm, new_frame->BP + i));

	// Increment caller's PC
	frame->PC += 5;
}

void op_RET(VM *vm, Frame *frame) {
	vm->CallStack.Depth -= 1;
	if (frame->SP > frame->BP) {
		u32 val = Load(vm, frame->SP - 1); // load return value
		TRACE("[=%d]", val);
		Frame *caller = CURRENT_FRAME(vm);
		Store(vm, caller->SP++, val); // push it onto caller's stack
	}
}

void op_HALT(VM *vm, Frame *frame) {
	vm->Flags |= VMFLAG_HALT;
	frame->PC++;
}

void op_PANIC(VM *vm, Frame *frame) {
	VM_Panic(vm, "software panic");
}

void op_BRK(VM *vm, Frame *frame) {
	vm->Flags |= VMFLAG_BREAKPOINT;
	frame->PC++;
}