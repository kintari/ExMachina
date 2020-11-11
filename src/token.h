#pragma once

#include "types.h"

#define MAX_TOKEN 1024

#define TOKEN_TYPES \
	X(None, 0) \
	X(Unexpected, 0) \
	X(EndOfStream, 0) \
	X(KeywordFunction, "function") \
	X(KeywordReturn, "return") \
	X(KeywordIf, "if") \
	X(KeywordElse, "else") \
	X(KeywordInt, "int") \
	X(KeywordUint, "uint") \
	X(Identifier, 0) \
	X(IntegerLiteral, 0) \
	X(StringLiteral, 0) \
	X(LParen, "(") \
	X(RParen, ")") \
	X(LBrace, "{") \
	X(RBrace, "}") \
	X(Colon, ":") \
	X(Comma, ",") \
	X(Semicolon, ";") \
	X(Equals, "=") \
	X(CompareEq, "==") \
	X(CompareNotEq, "!=") \
	X(Plus, "+") \
	X(Minus, "-") \

typedef enum {
#define X(A,B) Token_ ## A,
    TOKEN_TYPES
#undef X
} TokenType;

typedef struct Token {
    TokenType Type;
    u32 Line, Column, Pos;
    const u8 *Text;
    u32 Length;
} Token;

#define TOKEN_EMPTY ((Token){ 0, 0, 0, 0, 0, 0 })

const char *TokenType_ToString(TokenType type);

TokenType TokenType_FromString(const char *str, size_t len);