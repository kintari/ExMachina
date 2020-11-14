
#include "eval.h"
#include "io.h"
#include "str.h"
#include "trace.h"

#include <stdlib.h>
#include <assert.h>

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

AstEvalVisitor *AstEvalVisitor_New() {
	AstEvalVisitor *v = calloc(1, sizeof(AstEvalVisitor));
	v->Frame = PushFrame(v); // FIXME: no null functions
	Scope *scope = CurrentScope(v);

	Object *println = calloc(1, sizeof(Object));
	println->OpInvoke = io_println_OpInvoke;
	println->Self = NULL;
	Value value = { .Type = Value_Object, .Object = println };
	PutSymbol(scope, &(String) { .Bytes = "println", .Length = 7 }, &value);

	return v;
}

void AstEvalVisitor_Eval(AstEvalVisitor *v, const AstNode *node) {
	eval(v, node);
}

u32 NumOperands(const AstEvalVisitor *v) {
	assert(v->Frame);
	return (u32) v->Frame->NumOperands;
}

bool GetOperand(AstEvalVisitor *v, int index, Value *valuep, int *status) {
	assert(v->Frame);
	if (index >= 0 && index < v->Frame->NumOperands) {
		*valuep = v->Frame->Operands[v->Frame->NumOperands - index - 1];
		*status = 0;
		return true;
	}
	else if (index < 0 && -index <= v->Frame->NumOperands) {
		*valuep = v->Frame->Operands[-(index+1)];
		*status = 0;
		return true;
	}
	else {
		*status = -1;
		return false;
	}
}