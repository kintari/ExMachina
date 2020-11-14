#pragma once

#include "ast.h"

typedef struct AstEvalVisitor AstEvalVisitor;

typedef void (*FnInvoke)(AstEvalVisitor *, void *);

typedef struct Object {
	void *Self;
	FnInvoke OpInvoke;
} Object;

typedef enum {
	Value_None,
	Value_Uint,
	Value_Function,
	Value_Object
} ValueType;

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

typedef struct AstEvalVisitor {
	struct Activation *Frame;
} AstEvalVisitor;

AstEvalVisitor *AstEvalVisitor_New();

void AstEvalVisitor_Eval(AstEvalVisitor *, const AstNode *);

u32 NumOperands(const AstEvalVisitor *);

bool GetOperand(AstEvalVisitor *, int index, Value *value, int *status);