#pragma once

#include "types.h"

#define MAX_TOKEN 1024

#define TOKEN_TYPES \
    X(None, "none") \
    X(EOF, "eof") \
    X(KeywordFunction, "function") \
    X(KeywordInt, "int") \
    X(KeywordUint, "uint") \
    X(LParen, "(") \
    X(RParen, ")") \
    X(LBrace, "{") \
    X(RBrace, "}") \
    X(Colon, ":") \
    X(Comma, ",")

typedef enum {
#define X(A,B) Token_ ## A,
    TOKEN_TYPES
#undef X
} TokenType;

typedef struct Token {
    TokenType Type;
    u32 Line, Column;
} Token;

