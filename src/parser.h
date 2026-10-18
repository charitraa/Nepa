#ifndef NEPLANG_PARSER_H
#define NEPLANG_PARSER_H

#include "lexer.h"
#include "ast.h"

typedef struct {
    Lexer *lexer;
    const char *source;
    const char *filename;
    Token current;
    int had_error;
} Parser;

void parser_init(Parser *parser, Lexer *lexer, const char *source, const char *filename);
AstNode *parser_parse_program(Parser *parser);

#endif
