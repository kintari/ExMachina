
#include "token.h"

#define PASTE(x,y) x ## y
#define X(y,z) PASTE("Token_", #y),


static const char *TYPE_STRINGS[] = {
    TOKEN_TYPES
};
#undef X

const char *TokenType_ToString(TokenType type) {
    return TYPE_STRINGS[type];
}