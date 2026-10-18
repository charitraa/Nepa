#ifndef NEPLANG_LEXER_H
#define NEPLANG_LEXER_H

#include "token.h"

typedef struct {
    const char *source;
    const char *current;
    int line;
    int col;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);

#endif
