#pragma once

#include "scanner.h"
#include "ast.h"

typedef struct Parser Parser;

Parser *Parser_New(Scanner *);

AstNode *Parser_BuildAst(Parser *);