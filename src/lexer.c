#include <ctype.h>
#include <string.h>
#include "lexer.h"

void lexer_init(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->current = source;
    lexer->line = 1;
    lexer->col = 1;
}

static char peek(Lexer *lexer) {
    return *lexer->current;
}

static char advance(Lexer *lexer) {
    char c = *lexer->current;
    lexer->current++;
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

static void skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        if (c == ' ' || c == '\t' || c == '\r') {
            advance(lexer);
        } else if (c == '#') {
            while (peek(lexer) != '\n' && peek(lexer) != '\0') {
                advance(lexer);
            }
        } else {
            return;
        }
    }
}

static Token make_token(TokenType type, const char *start, int length, int line, int col) {
    Token token;
    token.type = type;
    token.start = start;
    token.length = length;
    token.value = 0;
    token.line = line;
    token.col = col;
    return token;
}

static Token number_token(Lexer *lexer) {
    const char *start = lexer->current;
    int line = lexer->line;
    int col = lexer->col;

    while (isdigit((unsigned char)peek(lexer))) {
        advance(lexer);
    }

    int length = (int)(lexer->current - start);
    Token token = make_token(TOKEN_INT, start, length, line, col);

    long value = 0;
    for (int i = 0; i < length; i++) {
        value = value * 10 + (start[i] - '0');
    }
    token.value = value;
    return token;
}

static Token ident_token(Lexer *lexer) {
    const char *start = lexer->current;
    int line = lexer->line;
    int col = lexer->col;

    while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    int length = (int)(lexer->current - start);
    TokenType type = TOKEN_IDENT;
    if (length == 4 && strncmp(start, "vana", 4) == 0) {
        type = TOKEN_VANA;
    }
    return make_token(type, start, length, line, col);
}

Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    const char *start = lexer->current;
    int line = lexer->line;
    int col = lexer->col;

    char c = peek(lexer);

    if (c == '\0') {
        return make_token(TOKEN_EOF, start, 0, line, col);
    }

    if (isdigit((unsigned char)c)) {
        return number_token(lexer);
    }

    if (isalpha((unsigned char)c) || c == '_') {
        return ident_token(lexer);
    }

    advance(lexer);
    switch (c) {
        case '+': return make_token(TOKEN_PLUS, start, 1, line, col);
        case '-': return make_token(TOKEN_MINUS, start, 1, line, col);
        case '*': return make_token(TOKEN_STAR, start, 1, line, col);
        case '/': return make_token(TOKEN_SLASH, start, 1, line, col);
        case '%': return make_token(TOKEN_PERCENT, start, 1, line, col);
        case '(': return make_token(TOKEN_LPAREN, start, 1, line, col);
        case ')': return make_token(TOKEN_RPAREN, start, 1, line, col);
        case '=': return make_token(TOKEN_EQUAL, start, 1, line, col);
        case '\n': return make_token(TOKEN_NEWLINE, start, 1, line, col);
        default:  return make_token(TOKEN_ERROR, start, 1, line, col);
    }
}
