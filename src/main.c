#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "vm.h"
#include "module.h"
#include "scanner.h"
#include "token.h"
#include "trace.h"
#include "parser.h"
#include "ast.h"
#include "eval.h"

void printToken(const Token *token) {
    const char *type = TokenType_ToString(token->Type);
    TRACE("Token { .Type=%s, .Text='%.*s', .Line=%d, .Column=%d }", type, token->Text.Length, token->Text.Bytes, token->Line, token->Column);
}

void scan(const u8 *buf, size_t size) {
    Scanner *scanner = Scanner_New(buf, (u32) size);
    Token token;
    TRACE("<begin token list>");
    while (Scanner_ReadNext(scanner, &token))
        printToken(&token);
	 TRACE("<end token list>");
}

char *fill(char *buf, char ch, int count) {
	count = count < 0 ? 0 : count;
	for (int i = 0; i < count; i++)
		buf[i] = ch;
	buf[count] = '\0';
	return buf;
}

char *indent(char *buf, int count) {
	return fill(buf, ' ', 2 * count);
}

#define TOKEN(x) (x).Text.Length, (x).Text.Bytes

#define VISITOR_VTBL_FNS(T) \
	X(Function, T) \
	X(If,T) \
	X(Block,T) \
	X(Return,T) \
	X(Expression,T) \
	X(Literal,T) \
	X(Identifier,T) \
	X(FunctionCall,T)

typedef struct AstVisitor AstVisitor;
typedef struct PrintVisitor PrintVisitor;

#define X(f,T) void (* ## f ## )(T *, const Ast ## f ## Node *);

typedef struct AstVisitorVtbl {
	VISITOR_VTBL_FNS(AstVisitor)
} AstVisitorVtbl;

typedef struct PrintVisitorVtbl {
	VISITOR_VTBL_FNS(PrintVisitor)
} PrintVisitorVtbl;

#undef X

typedef struct AstVisitor {
	const AstVisitorVtbl *Vfptr;
} AstVisitor;

typedef struct PrintVisitor {
	const PrintVisitorVtbl *Vfptr;
	int _indent;
} PrintVisitor;


void print(const AstNode *node, int level) {
	char buf[512];
	char *ind = indent(buf, level);
	level += 1;
	if (node != NULL) {
		switch (node->Type) {
			case AstNode_Module: {
				AST_FOREACH(x, AST_CAST(AstModuleNode, node)->Statements, print(x, level));
				break;
			}
			case AstNode_Function: {
				AstFunctionNode *fn = AST_CAST(AstFunctionNode, node);
				if (fn->ReturnType.Type == Token_None)
					TRACE("%s[function: %.*s]", ind, TOKEN(fn->Identifier));
				else
					TRACE("%s[function: %.*s -> %.*s]", ind, TOKEN(fn->Identifier), TOKEN(fn->ReturnType));
				AST_FOREACH(x, AST_CAST(AstNode, fn->Body), print(x, level));
				TRACE("%s[/function: %.*s]", ind, TOKEN(fn->Identifier));
				break;
			}
			case AstNode_If: {
				AstIfNode *iff = AST_CAST(AstIfNode, node);
				TRACE("%s[if]", ind);
				TRACE("%s[condition]", indent(buf, level));
				print(iff->Condition, level + 1);
				TRACE("%s[/condition]", indent(buf, level));
				TRACE("%s[if-true]", indent(buf, level));
				print(iff->TrueBranch, level + 1);
				TRACE("%s[/if-true]", indent(buf, level));
				TRACE("%s[if-false]", indent(buf, level));
				print(iff->FalseBranch, level + 1);
				TRACE("%s[/if-false]", ind);
				break;
			}
			case AstNode_Block: {
				TRACE("%s[block]", ind);
				AST_FOREACH(x, AST_CAST(AstBlockNode, node)->Statements, print(x, level));
				TRACE("%s[/block]", ind);
				break;
			}
			case AstNode_Return: {
				TRACE("%s[return]", ind);
				print((AstNode *) AST_CAST(AstReturnNode, node)->Expression, level);
				TRACE("%s[/return]", ind);
				break;
			}
			case AstNode_Expression: {
				AstExpressionNode *expr = AST_CAST(AstExpressionNode, node);
				TRACE("%s[expression: %.*s]", ind, TOKEN(expr->Operator));
				print(node->Left, level);
				print(node->Right, level);
				TRACE("%s[/expression]", ind, TOKEN(expr->Operator));
				break;
			}
			case AstNode_Literal: {
				const AstLiteralNode *lit = AST_CAST(AstLiteralNode, node);
				const char *quote = lit->Token.Type == Token_StringLiteral ? "'" : "";
				TRACE("%s[literal: %s%.*s%s]", ind, quote, TOKEN(lit->Token), quote);
				break;
			}
			case AstNode_Identifier: {
				TRACE("%s[identifier: %.*s]", ind, TOKEN(AST_CAST(AstIdentifierNode, node)->Token));
				break;
			}
			case AstNode_FunctionCall: {
				char tmp[256];
				indent(tmp, level);
				TRACE("%s[function-call]", ind);
				AstFunctionCallNode *fncall = AST_CAST(AstFunctionCallNode, node);
				print(fncall->Function, level);
				int i = 0;
				AstNode *arg = fncall->Arguments;
				while (arg) {
					i++;
					arg = arg->Right;
				}
				TRACE("%s[arguments: %d]", tmp, i);
				AST_FOREACH(x, fncall->Arguments, print(x->Left, level + 1));
				TRACE("%s[/arguments]", tmp);
				TRACE("%s[/function-call]", ind);
				break;
			}
			default: {
				TRACE("%s[unknown] %d", ind, node->Type);
				break;
			}
		}
	}
}

void parse(const u8 *buf, size_t size) {
	Scanner *s = Scanner_New(buf, (u32)size);
	Parser *p = Parser_New(s);
	AstNode *program = Parser_BuildAst(p);
	print(program, 0);
	
	AstEvalVisitor *v = AstEvalVisitor_New();
	AstEvalVisitor_Eval(v, program);
}

int main(int argc, const char *argv[]) {
    FILE *file = NULL;
    const char *filename = "scripts/fib.vm";
    if (fopen_s(&file, filename, "rb") != 0) {
        fprintf(stderr, "could not open file '%s' for reading\n", filename);
    }
    else {
        struct stat st;
        if (fstat(_fileno(file), &st) == 0) {
            if (st.st_size > 0) {
                u8 *buf = malloc(st.st_size + 1);
                if (buf) {
                    if (fread(buf, 1, st.st_size, file) == st.st_size) {
                        scan(buf, st.st_size);
								parse(buf, st.st_size);
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