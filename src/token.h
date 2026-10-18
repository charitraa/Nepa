#ifndef NEPA_TOKEN_H
#define NEPA_TOKEN_H

typedef enum {
    TOKEN_INT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EQUAL,
    TOKEN_NEWLINE,
    TOKEN_IDENT,
    TOKEN_VANA,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    long value;
    int line;
    int col;
} Token;

#endif
