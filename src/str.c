#include "str.h"
#include "types.h"

#include <stdlib.h>

char *strndup(const char *str, size_t len) {
	char *r = malloc(len + 1);
	for (size_t i = 0; i < len; i++)
		r[i] = str[i];
	r[len] = '\0';
	return r;
}
