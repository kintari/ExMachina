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

#define AST_FOREACH(var,node,func) \
	do { \
		AstNode *var = node; \
		while (var) { \
			AstNode *next = var->Right; \
			(func); \
			var = next; \
		} \
	}  while (0)

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

typedef struct AstEvalVisitor {
	struct Activation *Frame;
} AstEvalVisitor;

typedef enum {
	Value_None,
	Value_Uint,
	Value_Function,
	Value_Object
} ValueType;

typedef void (*FnInvoke)(AstEvalVisitor *, void *);

typedef struct Object {
	void *Self;
	FnInvoke OpInvoke;
} Object;

typedef struct Value {
	ValueType Type;
	union {
		Object *Object;
		const void *Pointer;
		uint64_t Uint;
	};
} Value;

typedef struct Symbol {
	String Identifier;
	Value Value;
} Symbol;

typedef struct Scope {
	struct Symbol *Symbols[256];
	size_t NumSymbols;
} Scope;

typedef struct Activation {
	const AstFunctionNode *Function;
	Scope Scopes[256];
	size_t NumScopes;
	Value Operands[256];
	size_t NumOperands;
	struct Activation *Next;
} Activation;

int PutSymbol(Scope *scope, const String *identifier, const Value *value) {
	// TODO: need to see if duplicate
	Symbol *symbol = calloc(1, sizeof(Symbol));
	symbol->Identifier = String_Copy(identifier);
	symbol->Value = *value;
	scope->Symbols[scope->NumSymbols++] = symbol;
	return 0;
}

Symbol *GetSymbol(AstEvalVisitor *v, const String *identifier) {
	assert(v->Frame);
	for (Activation *frame = v->Frame; frame; frame = frame->Next) {
		assert(frame->NumScopes > 0);
		for (size_t j = frame->NumScopes; j > 0; j--) {
			Scope *scope = &frame->Scopes[j-1];
			for (size_t k = 0; k < scope->NumSymbols; k++) {
				if (String_Equals(&scope->Symbols[k]->Identifier, identifier))
					return scope->Symbols[k];
			}
		}
	}
	return NULL;
}

Scope *CurrentScope(AstEvalVisitor *v) {
	assert(v->Frame->NumScopes > 0);
	return &v->Frame->Scopes[v->Frame->NumScopes - 1];
}

Scope *PushScope(AstEvalVisitor *v) {
	assert(v->Frame);
	Scope *scope = &v->Frame->Scopes[v->Frame->NumScopes++];
	scope->NumSymbols = 0;
	return scope;
}

void PopScope(AstEvalVisitor *v) {
	assert(v->Frame);
	assert(v->Frame->NumScopes > 0);
	Scope *scope = &v->Frame->Scopes[--v->Frame->NumScopes];
	for (size_t j = 0; j < scope->NumSymbols; j++) {
		Symbol *symbol = scope->Symbols[j];
		free(symbol->Identifier.Bytes);
	}
}

Activation *PushFrame(AstEvalVisitor *v) {
	Activation *frame = calloc(1, sizeof(Activation));
	frame->NumScopes = 1;
	Scope *scope = &frame->Scopes[0];
	scope->NumSymbols = 0;
	frame->Next = v->Frame;
	v->Frame = frame;
	return frame;
}

void PopFrame(AstEvalVisitor *v) {
	Activation *frame = v->Frame;
	while (frame->NumScopes > 0) {
		PopScope(v);
	}
	v->Frame = frame->Next;
	free(frame);
}

void PushOperand(AstEvalVisitor *v, const Value *valuep) {
	v->Frame->Operands[v->Frame->NumOperands++] = *valuep;
}

bool PopOperand(AstEvalVisitor *v, Value *valuep) {
	if (v->Frame->NumOperands > 0) {
		if (valuep) *valuep = v->Frame->Operands[v->Frame->NumOperands - 1];
		--v->Frame->NumOperands;
		return true;
	}
	else {
		return false;
	}
}

static bool ToBoolean(const Value *value) {
	switch (value->Type) {
		case Value_Uint:
			return value->Uint != 0;
		case Value_Object:
			return value->Object != NULL;
		default:
			abort();
	}
}

void eval(AstEvalVisitor *v, const AstNode *node);

void eval_Function(AstEvalVisitor *v, const AstFunctionNode *function) {
	Object *object = calloc(1, sizeof(Object));
	object->Self = function->Body;
	object->OpInvoke = eval;
	PutSymbol(CurrentScope(v), &function->Identifier.Text, &(Value) { .Type = Value_Object, .Object = object });
}

void eval_FunctionCall(AstEvalVisitor *v, const AstFunctionCallNode *node) {

	// Evaluate the expression that resolves to target function
	eval(v, node->Function);

	// Get the result off the stack
	Value operand;
	if (PopOperand(v, &operand)) {
		if (operand.Type != Value_Object) {
			// raise a type error
			abort();
		}
		else if (operand.Object->OpInvoke == NULL) {
			// another kind of error
			abort();
		}
		else {
			Activation *caller = v->Frame;

			// Evaluate function arguments
			const AstNode *argument = node->Arguments;
			int count = 0;
			while (argument) {
				eval(v, argument->Left);
				const AstNode *next = argument->Right;
				++count;
				argument = next;
			}

			Activation *callee = PushFrame(v);

			// Move operands into callee's frame
			for (int i = 0; i < count; i++) {
				callee->Operands[callee->NumOperands++] = caller->Operands[--caller->NumOperands];
			}

			// Evaluate function in callee context
			Object *object = operand.Object;
			operand.Object->OpInvoke(v, object->Self);

			// Push the return value onto the caller's stack
			Value returnValue;
			if (PopOperand(v, &returnValue)) {
				caller->Operands[caller->NumOperands++] = returnValue;
			}

			// Pop callee frame
			PopFrame(v);
		}
	}
}

void eval_Return(AstEvalVisitor *v, const AstReturnNode *node) {
	
}

void eval_Module(AstEvalVisitor *v, const AstModuleNode *module) {
	AST_FOREACH(stmt, module->Statements, eval(v, stmt));
}

void eval_Declaration(AstEvalVisitor *v, const AstDeclarationNode *node) {

}

void eval_Block(AstEvalVisitor *v, const AstBlockNode *block) {
	PushScope(v);
	for (AstNode *statement = block->Statements; statement != NULL; statement = statement->Right) {
		eval(v, statement);
		while (v->Frame->NumOperands > 0)
			PopOperand(v, NULL);
	}
	PopScope(v);
}

void eval_If(AstEvalVisitor *v, const AstIfNode *node) {
	assert(node->Condition);
	eval(v, node->Condition);
	Value result;
	if (PopOperand(v, &result)) {
		bool b = ToBoolean(&result);
		if (b) {
			eval(v, node->TrueBranch);
		}
		else {
			eval(v, node->FalseBranch);
		}
	}
}

void eval_Identifier(AstEvalVisitor *v, const AstIdentifierNode *node) {
	Symbol *sym = GetSymbol(v, &node->Token.Text);
	if (!sym) {
		TRACE("failed to resolve symbol '%.*s'", node->Token.Text.Length, (const char *)node->Token.Text.Bytes);
		abort(); // TODO: this is a legitimate runtime error, need to figure out how to handle it
	}
	else {
		Activation *frame = v->Frame;
		frame->Operands[frame->NumOperands++] = sym->Value;
	}
}

void eval_Literal(AstEvalVisitor *v, const AstLiteralNode *node) {
	long x = strtol(node->Token.Text.Bytes, NULL, 10);
	Activation *frame = v->Frame;
	frame->Operands[frame->NumOperands++] = (Value) { .Type = Value_Uint, .Uint = x };
}

void eval_Expression(AstEvalVisitor *v, const AstExpressionNode *expr) {
	
}

void eval(AstEvalVisitor *v, const AstNode *node) {
	if (node != NULL) {
		switch (node->Type) {
			case AstNode_Function:
				eval_Function(v, (const AstFunctionNode *) node);
				break;
			case AstNode_FunctionCall:
				eval_FunctionCall(v, (const AstFunctionCallNode *) node);
				break;
			case AstNode_Return:
				break;
			case AstNode_Module:
				break;
			case AstNode_Declaration:
				break;
			case AstNode_Block:
				eval_Block(v, (const AstBlockNode *) node);
				break;
			case AstNode_If:
				eval_If(v, (const AstIfNode *) node);
				break;
			case AstNode_Identifier:
				eval_Identifier(v, (const AstIdentifierNode *) node);
				break;
			case AstNode_Literal:
				eval_Literal(v, (const AstLiteralNode *) node);
				break;
			case AstNode_Expression:
				eval_Expression(v, (const AstExpressionNode *) node);
				break;
			default:
				break;
		}
	}
}

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

void io_println_OpInvoke(AstEvalVisitor *v, void *unused) {
	char buf[1024];
	char *ptr = buf;
	char *endp = buf + sizeof(buf);
	size_t offset = 0;
	while(v->Frame->NumOperands) {
		Value operand;
		PopOperand(v, &operand);
		switch (operand.Type) {
			case Value_Uint: {
				int nc = snprintf(ptr, endp - ptr, "%zu", operand.Uint);
				if (nc > 0)
					ptr += nc;
				if (v->Frame->NumOperands > 0) {
					*ptr = ' '; ptr++;
					*ptr = '\0';
				}
				break;
			}
		}
	}
	TRACE("%s", buf);
}


void parse(const u8 *buf, size_t size) {
	Scanner *s = Scanner_New(buf, (u32)size);
	Parser *p = Parser_New(s);
	AstNode *program = Parser_BuildAst(p);
	print(program, 0);
	
	AstEvalVisitor *v = calloc(1, sizeof(AstEvalVisitor));
	v->Frame = PushFrame(v); // FIXME: no null functions
	Scope *scope = CurrentScope(v);
	
	Object *println = calloc(1, sizeof(Object));
	println->OpInvoke = io_println_OpInvoke;
	println->Self = NULL;
	Value value = { .Type = Value_Object, .Object = println };
	PutSymbol(scope, &(String) { .Bytes = "println", .Length = 7 }, &value);
	eval(v, program);
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