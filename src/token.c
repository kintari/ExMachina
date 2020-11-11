
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

TokenType TokenType_FromString(const char *str, size_t len) {
	for (int i = 0; i < countof(STRINGS_TO_TYPES); i++)
		if (STRINGS_TO_TYPES[i].Text && strncmp(STRINGS_TO_TYPES[i].Text, str, len) == 0)
			return STRINGS_TO_TYPES[i].Type;
	return Token_None;
}