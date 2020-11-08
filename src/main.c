#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vm.h"
#include "module.h"
#include "scanner.h"
#include "token.h"

void printToken(const Token *token) {
    const char *type = TokenType_ToString(token->Type);
    printf("Token { .Type=%s, .Text='%.*s', .Line=%d, .Column=%d }\n", type, token->Length, token->Text, token->Line, token->Column);
}

void scan(const u8 *buf, size_t size) {
    Scanner *scanner = Scanner_New(buf, (u32) size);
    Token token;
    printf("<begin token list>\n");
    while (Scanner_ReadNext(scanner, &token))
        printToken(&token);
    printf("<end token list>\n");
}

int main(int argc, const char *argv[]) {
    FILE *file = NULL;
    const char *filename = "scripts/fib.vm";

    if (fopen_s(&file, filename, "rb") != 0) {
        fprintf(stderr, "could not open file '%s' for reading\n", filename);
        exit(-1);
    }
    else {
        struct stat st;
        if (fstat(_fileno(file), &st) == 0) {
            if (st.st_size > 0) {
                u8 *buf = malloc(st.st_size + 1);
                if (buf) {
                    if (fread(buf, 1, st.st_size, file) == st.st_size) {
                        scan(buf, st.st_size);                        
                    }
                    free(buf);
                }
            }
            
        }
        fclose(file);
    }

#if 0
	VM vm;
	memset(&vm, 0, sizeof(vm));
	vm.Module = LoadModule();
	const Function *global = &vm.Module->Functions[0];
	Frame frame = (Frame){.Function = global, .PC = 0, .BP = 0, .SP = 0};
	vm.CallStack.Frames[vm.CallStack.Depth++] = frame;
	while ((vm.Flags & VMFLAG_HALT) == 0)
		VM_Run(&vm);
#endif

	return 0;
}