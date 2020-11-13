
#include "parser.h"
#include "debug.h"
#include "trace.h"

#include <stdlib.h>
#include <string.h>

typedef enum {
	ParseError_None,
	ParseError_Expected,
	ParseError_Unexpected
} ParseErrorType;

struct Parser {
	Scanner *Scanner;
	Token Token;
	ParseErrorType Error;
};

#define AST_NEW(T) calloc(1, sizeof(T))
#define AST_NODE(X) (&(X)->Base)

Parser *Parser_New(Scanner *scanner) {
	Parser *p = calloc(1, sizeof(Parser));
	p->Scanner = scanner;
	p->Token = TOKEN_EMPTY;
	p->Error = 0;
	Scanner_ReadNext(p->Scanner, &p->Token);
	return p;
}

static void SetError(Parser *p, int error) {
	if (p->Error == 0)
		p->Error = error;
}

static bool Peek(Parser *p, TokenType type, Token *token) {
	bool match = p->Error == ParseError_None && p->Token.Type == type;
	if (match && token) {
		*token = p->Token;
	}
	return match;
}

static bool Match(Parser *p, TokenType type, Token *token) {
	bool match = Peek(p, type, token);
	if (match) {
		TRACE("[parser] matched %s -> '%.*s'", TokenType_ToString(type), p->Token.Text.Length, (const char *) p->Token.Text.Bytes);
		Scanner_ReadNext(p->Scanner, &p->Token);
	}
	return match;
}

static bool MatchType(Parser *p, Token *token) {
	return Match(p, Token_KeywordInt, token) || Match(p, Token_KeywordUint, token);
}

static bool MatchBinaryOperator(Parser *p, Token *token) {
	TokenType types[] = { Token_CompareEq, Token_CompareNotEq, Token_Plus, Token_Minus };
	for (size_t i = 0; i < countof(types); i++)
		if (Match(p, types[i], token))
			return true;
	return false;
}

static AstBlockNode *StatementBlock(Parser *p);
static AstNode *Statement(Parser *p);
static AstNode *Expression(Parser *p);
static AstNode *ArgumentList(Parser *p);

static AstIdentifierNode *Identifier(Parser *p) {
	Token token;
	if (Match(p, Token_Identifier, &token)) {
		AstIdentifierNode *node = AST_NEW(AstIdentifierNode);
		*node = (AstIdentifierNode) { {.Type = AstNode_Identifier }, .Token = token };
		return node;
	}
	return NULL;
}

static AstLiteralNode *Literal(Parser *p) {
	Token token;
	if (Match(p, Token_IntegerLiteral, &token) || Match(p, Token_StringLiteral, &token)) {
		AstLiteralNode *node = AST_NEW(AstLiteralNode);
		*node = (AstLiteralNode) { { .Type = AstNode_Literal }, .Token = token };
		return node;
	}
	return NULL;
}

static AstExpressionNode *MakeBinaryExpression(AstNode *lhs, AstNode *rhs, const Token *oper) {
	AstExpressionNode *expr = AST_NEW(AstExpressionNode);
	*expr = (AstExpressionNode) { {.Type = AstNode_Expression, .Left = lhs, .Right = rhs }, .Operator = *oper };
	TRACE("[parser] returning expression");
	return expr;
}

AstNode *Subexpression(Parser *p) {
	if (Match(p, Token_LParen, NULL)) {
		AstNode *node = Expression(p);
		if (Match(p, Token_RParen, NULL)) {
			return node;
		}
		else {
			SetError(p, -1);
		}
	}
	return NULL;
}


static AstNode *Factor(Parser *p) {
	AstNode *node = AST_NODE(Literal(p));
	if (!node) {
		node = AST_NODE(Identifier(p));
	}
	if (!node) {
		node = Subexpression(p);
	}
	if (Match(p, Token_LParen, NULL)) {
		AstNode *arguments = ArgumentList(p);
		if (Match(p, Token_RParen, NULL)) {
			AstFunctionCallNode *fn = AST_NEW(AstFunctionCallNode);
			*fn = (AstFunctionCallNode) { {.Type = AstNode_FunctionCall }, .Arguments = arguments, .Function = node, };
			return AST_NODE(fn);
		}
		else {
			SetError(p, -1);
			return NULL;
		}
	}
	return node;
}

static AstNode *Term(Parser *p) {
	AstNode *node = Factor(p);
	if (node) {
		Token oper;
		if (Match(p, Token_Multiply, &oper) || Match(p, Token_Divide, &oper)) {
			AstNode *tail = Factor(p);
			if (tail) {
				return AST_NODE(MakeBinaryExpression(node, tail, &oper));
			}
			else {
				SetError(p, -1);
			}
		}
	}
	return node;
}

static AstNode *Expression(Parser *p) {
	// EXPR ::= EXPR-HEAD EXPR-TAIL
	// EXPR-HEAD ::= IDENTIFIER
	// EXPR-HEAD ::= LITERAL
	// EXPR-HEAD ::= true | false
	// EXPR-TAIL ::= BINARY-OPERATOR EXPR
	AstNode *node = Factor(p);
	if (node) {
		Token oper;
		if (MatchBinaryOperator(p, &oper)) {
			AstNode *tail = Expression(p);
			if (tail) {
				return AST_NODE(MakeBinaryExpression(node, tail, &oper));
			}
			else {
				SetError(p, -1);
			}
		}
	}
	return node;
}

static AstNode *ArgumentList(Parser *p) {
	AstNode *cell = AST_NEW(AstNode);
	AstNode *expr = Expression(p);
	cell->Left = expr;
	if (expr && Match(p, Token_Comma, NULL)) {
		AstNode *tail = ArgumentList(p);
		cell->Right = tail;
	}
	return cell;
}

static AstDeclarationNode *Parameter(Parser *p) {
	Token identifier;
	if (Match(p, Token_Identifier, &identifier)) {
		if (Match(p, Token_Colon, NULL)) {
			Token type;
			if (MatchType(p, &type)) {
				AstDeclarationNode *decl = AST_NEW(AstDeclarationNode);
				*decl = (AstDeclarationNode) { { .Type = AstNode_Declaration }, .Identifier = identifier, .Type = type };
				TRACE("[parser] returning parameter");
				return decl;
			}
		}
		SetError(p, -1);
	}
	return NULL;
}

static AstDeclarationNode *ParameterList(Parser *p) {
	AstDeclarationNode *parameter = Parameter(p);
	if (parameter) {
		parameter->Base.Right = (AstNode *) ParameterList(p);
	}
	return parameter;
}

static AstNode *Function(Parser *p) {
	if (Match(p, Token_KeywordFunction, NULL)) {
		Token name;
		if (Match(p, Token_Identifier, &name)) {
			if (Match(p, Token_LParen, NULL)) {
				AstDeclarationNode *parameters = ParameterList(p);
				if (Match(p, Token_RParen, NULL)) {
					if (Match(p, Token_Colon, NULL)) {
						Token returnType;
						if (MatchType(p, &returnType)) {
							AstBlockNode *body = StatementBlock(p);
							if (body) {
								AstFunctionNode *function = AST_NEW(AstFunctionNode);
								*function = (AstFunctionNode) { 
									{ .Type = AstNode_Function }, .Identifier = name, .Parameters = parameters, .ReturnType = returnType, .Body = body 
								};
								return AST_NODE(function);
							}
						}
					}
				}
			}
		}
	}
	SetError(p, -1);
	return NULL;
}

static AstNode *IfStatement(Parser *p) {
	if (Match(p, Token_KeywordIf, NULL)) {
		if (Match(p, Token_LParen, NULL)) {
			AstNode *condition = Expression(p);
			if (condition) {
				if (Match(p, Token_RParen, NULL)) {
					AstNode *trueBranch = Statement(p);
					AstNode *falseBranch = NULL;
					if (Match(p, Token_KeywordElse, NULL)) {
						falseBranch = Statement(p);
					}
					AstIfNode *node = AST_NEW(AstIfNode);
					*node = (AstIfNode) { { .Type = AstNode_If }, .Condition = condition, .TrueBranch = trueBranch, .FalseBranch = falseBranch };
					return AST_NODE(node);
				}
			}
		}
		SetError(p, -1);
	}
	return NULL;
}

static AstNode *ReturnStatement(Parser *p) {
	if (Match(p, Token_KeywordReturn, NULL)) {
		AstExpressionNode *expr = (AstExpressionNode *) Expression(p);
		if (Match(p, Token_Semicolon, NULL)) {
			AstReturnNode *node = AST_NEW(AstReturnNode);
			*node = (AstReturnNode) { {.Type = AstNode_Return }, .Expression = expr };
			return AST_NODE(node);
		}
		else {
			SetError(p, -1);
		}
	}
	return NULL;
}

static AstNode *Statement(Parser *p) {
	if (Peek(p, Token_KeywordFunction, NULL)) {
		return Function(p);
	}
	else if (Peek(p, Token_KeywordIf, NULL)) {
		return IfStatement(p);
	}
	else if (Peek(p, Token_LBrace, NULL)) {
		return (AstNode *) StatementBlock(p);
	}
	else if (Peek(p, Token_KeywordReturn, NULL)) {
		return (AstNode *) ReturnStatement(p);
	}
	else {
		AstNode *expr = Expression(p);
		return (expr && Match(p, Token_Semicolon, NULL)) ? expr : NULL;
	}
}

static AstNode *StatementList(Parser *p) {
	// STATEMENT-LIST ::= STATEMENT [STATEMENT-LIST]
	AstNode *statement = Statement(p);
	if (statement) {
		statement->Right = StatementList(p);
	}
	return statement;
}

static AstBlockNode *StatementBlock(Parser *p) {
	if (Match(p, Token_LBrace, NULL)) {
		AstNode *statements = StatementList(p);
		if (Match(p, Token_RBrace, NULL)) {
			AstBlockNode *block = AST_NEW(AstBlockNode);
			*block = (AstBlockNode) { { .Type = AstNode_Block }, .Statements = statements };
			block->Statements = statements;
			return block;
		}
		SetError(p, -1);
	}
	return NULL;
}

static AstNode *Module(Parser *p) {
	// MODULE ::= STATEMENT-LIST EOF (more to come later)
	AstNode *statements = StatementList(p);
	if (statements) {
		if (Match(p, Token_EndOfStream, NULL)) {
			//static const char *name = "$global";
			//AstFunctionNode *function = AST_NEW(AstFunctionNode);
			//function->Base.Type = AstNode_Function;
			//function->Identifier = (Token) { .Type = Token_Identifier, .Text = name, .Length = (u32) strlen(name) };
			//function->Body = AST_NEW(AstBlockNode);
			//*function->Body = (AstBlockNode) { { .Type = AstNode_Block }, .Statements = statements };
			AstBlockNode *block = calloc(1, sizeof(AstBlockNode));
			block->Base.Type = AstNode_Block;
			block->Statements = statements;
			return AST_NODE(block);
		}
	}
	return NULL;
}

AstNode *Parser_BuildAst(Parser *p) {
	return (AstNode *) Module(p);
}