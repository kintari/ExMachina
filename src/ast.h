#pragma once

#include "token.h"

#define AST_CAST(T,node) ((T *) (node))

#define AST_FOREACH(var,node,func) \
	do { \
		AstNode *var = node; \
		while (var) { \
			AstNode *next = var->Right; \
			(func); \
			var = next; \
		} \
	}  while (0)

typedef enum  {
	AstNode_None,
	AstNode_Function,
	AstNode_FunctionCall,
	AstNode_Return,
	AstNode_Module,
	AstNode_Declaration,
	AstNode_Block,
	AstNode_If,
	AstNode_Identifier,
	AstNode_Literal,
	AstNode_Expression
} AstNodeType;

typedef struct AstNode {
	AstNodeType Type;
	struct AstNode *Left, *Right;
} AstNode;

typedef struct AstIfNode {
	AstNode Base;
	AstNode *Condition;
	AstNode *TrueBranch, *FalseBranch;
} AstIfNode;

typedef struct AstIdentifierNode {
	AstNode Base;
	Token Token;
} AstIdentifierNode;

typedef struct AstLiteralNode {
	AstNode Base;
	Token Token;
} AstLiteralNode;

typedef struct AstExpressionNode {
	AstNode Base;
	Token Operator;
} AstExpressionNode;

typedef struct AstReturnNode {
	AstNode Base;
	AstExpressionNode *Expression;
} AstReturnNode;

typedef struct AstBlockNode {
	AstNode Base;
	AstNode *Statements;
} AstBlockNode;

typedef struct AstDeclarationNode {
	AstNode Base;
	Token Identifier, Type;
} AstDeclarationNode;

typedef struct AstFunctionNode {
	AstNode Base;
	Token Identifier, ReturnType;
	AstDeclarationNode *Parameters;
	AstBlockNode *Body;
} AstFunctionNode;

typedef struct AstFunctionCallNode {
	AstNode Base;
	AstNode *Function;
	AstNode *Arguments;
} AstFunctionCallNode;

typedef struct AstModuleNode {
	AstNode Base;
	AstNode *Statements;
} AstModuleNode;