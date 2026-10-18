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

static int match(Lexer *lexer, char expected) {
    if (peek(lexer) != expected) return 0;
    advance(lexer);
    return 1;
}

static const struct {
    const char *text;
    TokenType type;
} keywords[] = {
    { "vana",      TOKEN_VANA },
    { "yedi",      TOKEN_YEDI },
    { "natara",    TOKEN_NATARA },
    { "jabasamma", TOKEN_JABASAMMA },
    { "ko_lagi",   TOKEN_KO_LAGI },
    { "in",        TOKEN_IN },
};

static TokenType keyword_type(const char *start, int length) {
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].text) == (size_t)length && strncmp(start, keywords[i].text, (size_t)length) == 0) {
            return keywords[i].type;
        }
    }
    return TOKEN_IDENT;
}

static Token ident_token(Lexer *lexer) {
    const char *start = lexer->current;
    int line = lexer->line;
    int col = lexer->col;

    while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    int length = (int)(lexer->current - start);
    return make_token(keyword_type(start, length), start, length, line, col);
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
        case '.':
            if (match(lexer, '.')) return make_token(TOKEN_DOT_DOT, start, 2, line, col);
            return make_token(TOKEN_ERROR, start, 1, line, col);
        case '{': return make_token(TOKEN_LBRACE, start, 1, line, col);
        case '}': return make_token(TOKEN_RBRACE, start, 1, line, col);
        case '=':
            if (match(lexer, '=')) return make_token(TOKEN_EQUAL_EQUAL, start, 2, line, col);
            return make_token(TOKEN_EQUAL, start, 1, line, col);
        case '!':
            if (match(lexer, '=')) return make_token(TOKEN_BANG_EQUAL, start, 2, line, col);
            return make_token(TOKEN_ERROR, start, 1, line, col);
        case '<':
            if (match(lexer, '=')) return make_token(TOKEN_LESS_EQUAL, start, 2, line, col);
            return make_token(TOKEN_LESS, start, 1, line, col);
        case '>':
            if (match(lexer, '=')) return make_token(TOKEN_GREATER_EQUAL, start, 2, line, col);
            return make_token(TOKEN_GREATER, start, 1, line, col);
        case '\n': return make_token(TOKEN_NEWLINE, start, 1, line, col);
        default:  return make_token(TOKEN_ERROR, start, 1, line, col);
    }
}
