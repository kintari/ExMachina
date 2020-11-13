#pragma once

#include "types.h"

#include <string.h>

char *strndup(const char *, size_t);

typedef struct String {
	u8 *Bytes;
	size_t Length;
} String;

#define STRING_EMPTY { 0, 0 }

inline String String_Copy(const String *src) {
	return (String) {
		.Bytes = strndup((const char *)src->Bytes, src->Length),
		.Length = src->Length
	};
}

inline bool String_Equals(const String *x, const String *y) {
	return x->Length == y->Length && memcmp(x->Bytes, y->Bytes, x->Length) == 0;
}