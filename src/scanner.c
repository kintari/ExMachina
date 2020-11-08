
#include "scanner.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct Scanner {
    u32 Pos, Line, Column;
    const u8 *Text;
    u32 TextLength;
	 bool Done;
};

Scanner *Scanner_New(const u8 *text, u32 length) {
    Scanner *s = calloc(1, sizeof(Scanner));
    s->Text = text;
    s->TextLength = length;
    s->Line = 1;
    s->Column = 1;
    return s;
}

void Scanner_Release(Scanner *scanner) {
    free(scanner);
}

static bool Peek(Scanner *scanner, int *codePoint) {
    if (scanner->Pos < scanner->TextLength) {
        *codePoint = scanner->Text[scanner->Pos];
        return true;
    }
    else {
        *codePoint = -1;
        return false;
    }   
}

static bool Lookahead(Scanner *scanner, int *codePoint) {
    if (scanner->Pos + 1 < scanner->TextLength) {
        *codePoint = scanner->Text[scanner->Pos + 1];
        return true;
    }
    else {
        *codePoint = -1;
        return false;
    }   
}

static int NextChar(Scanner *scanner) {
    int ch = scanner->Text[scanner->Pos++];
    if (ch == '\n') {
        scanner->Line++;
        scanner->Column = 1;
    }
    else {
        scanner->Column++;
    }
    return ch;
}

static u32 TakeWhile(Scanner *scanner, int (*test)(int)) {
    int ch;
    u32 pos = scanner->Pos;
    while (Peek(scanner, &ch) && test(ch))
        NextChar(scanner);
    return scanner->Pos - pos;
}

static int IsIdentifierChar(int ch) {
   return isalpha(ch) || isdigit(ch) || ch == '_';
}

bool GetTokenType(TokenType *type, int ch) {
#define X(a,b) \
	case b: \
		*type = Token_ ## a; \
		return true; \

	switch (ch) {
		SINGLE_CHAR_TOKENS
		default:
			return false;
	}

#undef X
}

bool Scanner_ReadNext(Scanner *scanner, Token *token) {
    Token t;
    if (token == NULL)
        token = &t;
	 if (scanner->Done)
		 return false;
	 int ch, lk;
    while (Peek(scanner, &ch)) {
        u32 pos = scanner->Pos;
        token->Line = scanner->Line;
        token->Column = scanner->Column;
		  token->Text = &scanner->Text[scanner->Pos];
        token->Length = 0;
        if (isspace(ch)) {
            TakeWhile(scanner, isspace);
        }
        else if (isdigit(ch)) {
			  token->Length = TakeWhile(scanner, isdigit);
			  token->Type = Token_IntegerLiteral;
			  return true;
        }
		  else if (ch == '-' && Lookahead(scanner, &lk) && isdigit(lk)) {
			  NextChar(scanner);
			  token->Length = 1 + TakeWhile(scanner, isdigit);
			  token->Type = Token_IntegerLiteral;
			  return true;
		  }
        else if (isalpha(ch) || ch == '_') {
            token->Length = TakeWhile(scanner, IsIdentifierChar);
            token->Type = Token_Identifier;
            return true;
        }
        else if (GetTokenType(&token->Type, ch)) {
            token->Length = 1;
            NextChar(scanner);
            return true;
        }
		  else {
			  token->Length = 1;
			  token->Type = Token_Unknown;
			  NextChar(scanner);
			  return true;
		  }
    }
    token->Type = Token_EOF;
	 scanner->Done = true;
    return true;
}