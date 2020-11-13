
#include "token.h"

#include <string.h>

#define PASTE(x,y) x ## y
#define X(A,B) PASTE("Token_", #A),
static const char *TYPES_TO_STRINGS[] = {
    TOKEN_TYPES
};
#undef X

const char *TokenType_ToString(TokenType type) {
    return TYPES_TO_STRINGS[type];
}

static struct {
	TokenType Type;
	const char *Text;
} STRINGS_TO_TYPES[] = {
#define X(A,B) { Token_ ## A, B },
	TOKEN_TYPES
#undef X
};

TokenType TokenType_FromString(const String *str) {
	for (int i = 0; i < countof(STRINGS_TO_TYPES); i++) {
		const char *text = STRINGS_TO_TYPES[i].Text;
		if (text && strlen(text) == str->Length && strncmp(text, str->Bytes, str->Length) == 0) {
			return STRINGS_TO_TYPES[i].Type;
		}
	}
	return Token_None;
}