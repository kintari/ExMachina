#pragma once

#include <stdio.h>

#include "token.h"

typedef struct Scanner Scanner;

Scanner *Scanner_New(const u8 *text, u32 length);

void Scanner_Release(Scanner *);

bool Scanner_ReadNext(Scanner *, Token *);