#ifndef NEPLANG_INTERPRETER_H
#define NEPLANG_INTERPRETER_H

#include "ast.h"

void interpret(AstNode *program, const char *source, const char *filename);

#endif
