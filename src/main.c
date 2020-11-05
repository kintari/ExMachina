#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "vm.h"
#include "module.h"

int main(int argc, const char *argv[]) {
	VM vm;
	memset(&vm, 0, sizeof(vm));
	vm.Module = LoadModule();
	const Function *global = &vm.Module->Functions[0];
	Frame frame = (Frame){.Function = global, .PC = 0, .BP = 0, .SP = 0};
	vm.CallStack.Frames[vm.CallStack.Depth++] = frame;
	while ((vm.Flags & VMFLAG_HALT) == 0)
		VM_Run(&vm);
	return 0;
}