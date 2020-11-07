
#include "scanner.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const u8 *str(const u8 *value) {
    size_t len = strlen((const char *) value);
    u8 *r = malloc(len + 5);
    u8 *chars = &r[4];
    memcpy(chars, value, len);
    chars[len] = '\0';
    *((u32 *) r) = len;
    return chars;   
}

struct Scanner {
    FILE *File;
    u32 Pos, Line, Column, Peek, Lookahead;
};

Scanner *Scanner_New(FILE *file) {
    Scanner *s = calloc(1, sizeof(Scanner));
    s->File = file;
    s->Line = 1;
    s->Column = 1;
    s->Peek = fgetc(file);
    s->Lookahead = fgetc(file);
    return s;
}

void Scanner_Release(Scanner *scanner) {
    free(scanner);
}

bool ReadChar(Scanner *scanner, int *ch) {
    if (scanner->Peek == -1) {
        return false;
    }
    else {
        *ch = scanner->Peek;
        scanner->Peek = scanner->Lookahead;
        scanner->Lookahead = fgetc(scanner->File);
        return true;
    }
}

void Advance(Scanner *scanner) {
    int ch;
    ReadChar(scanner, &ch);
}

bool Scanner_ReadToken(Scanner *scanner, Token *token) {
    Token t;
    if (token == NULL)
        token = &t;
    if (scanner->Peek == -1) {
        return false;
    }
    while (scanner->Peek != -1) {
        u32 pos = scanner->Pos;
        token->Line = scanner->Line;
        token->Column = scanner->Column;
        TAKE_WHILE(scanner, isalpha(scanner->Peek) || isdigit(scanner->Peek) || scanner->Peek == '_');
    }
    return false;
}