#pragma once

#include <stdio.h>

#include "token.h"

typedef struct Scanner Scanner;

Scanner *Scanner_New();

void Scanner_Release(Scanner *);