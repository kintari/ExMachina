
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

static int IsIdentifierChar(int ch) {
   return isalpha(ch) || isdigit(ch) || ch == '_';
}

static int IsNotNewline(int ch) {
	return ch != '\n';
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

#define EVAL(x,y,z) PASTE(x,y,z)
#define PASTE(x,y,z) x ## y ## z

#define TAKE_WHILE(sc, tok, test) \
	do { \
		u32 pos = sc->Pos; \
		while (test) \
			NextChar(sc); \
		if (tok) tok->Length = sc->Pos - pos; \
	} while (0)

#define INIT_TOKEN(sc) \
	(Token) { .Line = scanner->Line, .Column = scanner->Column, .Text = &scanner->Text[scanner->Pos], .Length = 0 }

bool Scanner_ReadNext(Scanner *scanner, Token *token) {
    Token t;
    if (token == NULL)
        token = &t;
	 *token = INIT_TOKEN(scanner);
	 if (scanner->Done)
		 return false;
	 int ch, lk;
	 if (!Peek(scanner, &ch)) {
		 token->Type = Token_EOF;
		 scanner->Done = true;
		 return true;
	 }
	 while (Peek(scanner, &ch)) {
        u32 pos = scanner->Pos;
		  *token = INIT_TOKEN(scanner);
        if (isspace(ch)) {
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && isspace(ch));
        }
		  else if (ch == '/' && Lookahead(scanner, &lk) && lk == '/') {
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && ch != '\n');
		  }
		  else if (ch == '/' && Lookahead(scanner, &lk) && lk == '*') {
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && Lookahead(scanner, &lk) && (ch != '*' || lk != '/'));
			  NextChar(scanner);
			  NextChar(scanner);
		  }
        else if (isdigit(ch)) {
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && isdigit(ch));
			  token->Type = Token_IntegerLiteral;
			  return true;
        }
		  else if (ch == '-' && Lookahead(scanner, &lk) && isdigit(lk)) {
			  NextChar(scanner);
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && isdigit(ch));
			  token->Length += 1;
			  token->Type = Token_IntegerLiteral;
			  return true;
		  }
		  else if (ch == '"' || ch == '\'') {
			  int quote = ch;
			  NextChar(scanner);
			  token->Text++;
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && ch != quote);
			  NextChar(scanner);
			  token->Type = Token_StringLiteral;
			  return true;
		  }
        else if (isalpha(ch) || ch == '_') {
            TAKE_WHILE(scanner, token, Peek(scanner, &ch) && (isalpha(ch) || isdigit(ch) || ch == '_'));
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
			  token->Type = Token_Unexpected;
			  NextChar(scanner);
			  return true;
		  }
    }
    token->Type = Token_Unexpected;
	 scanner->Done = true;
    return true;
}