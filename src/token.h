#pragma once

#include "types.h"

#define MAX_TOKEN 1024

#define TOKEN_TYPES \
    X(None, "none") \
    X(Unknown, "unknown") \
    X(EOF, "eof") \
    X(KeywordFunction, "function") \
    X(KeywordInt, "int") \
    X(KeywordUint, "uint") \
    X(Identifier, "identifier") \
	 X(IntegerLiteral, "integer-literal") \
    X(LParen, "(") \
    X(RParen, ")") \
    X(LBrace, "{") \
    X(RBrace, "}") \
    X(Colon, ":") \
    X(Comma, ",") \
	 X(Semicolon, ';')

#define SINGLE_CHAR_TOKENS \
	X(LParen, '(') \
	X(RParen, ')') \
	X(LBrace, '{') \
	X(RBrace, '}') \
	X(Colon, ':') \
	X(Comma, ',') \
	X(Semicolon, ';')


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

const char *TokenType_ToString(TokenType type);