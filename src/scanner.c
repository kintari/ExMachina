
#include "scanner.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define TAKE_WHILE(sc, tok, test) \
	do { \
		u32 pos = sc->Pos; \
		while (test) \
			NextChar(sc); \
		if (tok) tok->Text.Length = sc->Pos - pos; \
	} while (0)

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

static bool Match1(Scanner *scanner, const char *set) {
	int pk;
	return Peek(scanner, &pk) && strchr(set, pk);
}

static bool Match2(Scanner *scanner, const char *set1, const char *set2) {
	int pk, la;
	if (Peek(scanner, &pk) && Lookahead(scanner, &la))
		return strchr(set1, pk) && strchr(set2, la);
	return false;
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

static void ScanNumericLiteral(Scanner *scanner, Token *token) {
	int ch;
	TAKE_WHILE(scanner, token, Peek(scanner, &ch) && isdigit(ch));
	token->Type = Token_IntegerLiteral;
}

void InitToken(Token *token, const Scanner *scanner) {
	token->Line = scanner->Line;
	token->Column = scanner->Column;
	token->Text = (String) { .Bytes = (u8 *) &scanner->Text[scanner->Pos], .Length = 0 };
}

bool Scanner_ReadNext(Scanner *scanner, Token *token) {
    Token t;
    if (token == NULL)
        token = &t;
	 InitToken(token, scanner);
	 if (scanner->Done)
		 return false;
	 int ch;
	 while (Peek(scanner, &ch)) {
        u32 pos = scanner->Pos;
		  InitToken(token, scanner);
        if (isspace(ch)) {
			  // whitespace
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && isspace(ch));
        }
		  else if (Match2(scanner, "/", "/")) {
			  // single line comment
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && ch != '\n');
		  }
		  else if (Match2(scanner, "/", "*")) {
			  // multi-line comment
			  TAKE_WHILE(scanner, token, !Match2(scanner, "*", "/"));
			  NextChar(scanner);
			  NextChar(scanner);
		  }
        else if (isdigit(ch)) {
			  // numeric literal
			  ScanNumericLiteral(scanner, token);
			  return true;
        }
		  else if (ch == '"' || ch == '\'') {
			  // string literal
			  char quote[2] = { ch, '\0' };
			  NextChar(scanner);
			  token->Text.Bytes++;
			  TAKE_WHILE(scanner, token, !Match1(scanner, quote));
			  NextChar(scanner);
			  token->Type = Token_StringLiteral;
			  return true;
		  }
		  else if (isalpha(ch) || ch == '_') {
			  // keyword or identifier
			  TAKE_WHILE(scanner, token, Peek(scanner, &ch) && (isalpha(ch) || isdigit(ch) || ch == '_'));
			  TokenType type = TokenType_FromString(&token->Text);
			  token->Type = type != Token_None ? type : Token_Identifier;
			  return true;
		  }
		  else if (Match2(scanner, "!=><|&", "=")) {
			  // comparison operators
			  token->Text.Length = 2;
			  token->Type = TokenType_FromString(&token->Text);
			  NextChar(scanner);
			  NextChar(scanner);
			  return true;
		  }
        else {
			  token->Text.Length = 1;
			  TokenType type = TokenType_FromString(&token->Text);
			  token->Type = type != Token_None ? type : Token_Unexpected;
			  NextChar(scanner);
			  return true;
        }
    }
	 token->Type = Token_EndOfStream;
	 scanner->Done = true;
	 return true;
}