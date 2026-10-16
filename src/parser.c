#include <string.h>
#include "parser.h"
#include "diagnostics.h"

static AstNode *parse_expr(Parser *parser);

static void advance(Parser *parser) {
    parser->current = lexer_next_token(parser->lexer);
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static void error_at_current(Parser *parser, const char *message) {
    if (parser->had_error) return;
    parser->had_error = 1;
    report_error(parser->source, parser->filename, parser->current, message);
}

static Token expect(Parser *parser, TokenType type, const char *message) {
    Token token = parser->current;
    if (!check(parser, type)) {
        error_at_current(parser, message);
        return token;
    }
    advance(parser);
    return token;
}

static AstNode *parse_factor(Parser *parser) {
    if (parser->had_error) return ast_new_number(0);

    if (check(parser, TOKEN_INT)) {
        long value = parser->current.value;
        advance(parser);
        return ast_new_number(value);
    }

    if (check(parser, TOKEN_MINUS) || check(parser, TOKEN_PLUS)) {
        char op = parser->current.start[0];
        advance(parser);
        AstNode *operand = parse_factor(parser);
        return ast_new_unary(op, operand);
    }

    if (check(parser, TOKEN_LPAREN)) {
        advance(parser);
        AstNode *inner = parse_expr(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' to close the expression.");
        return inner;
    }

    error_at_current(parser, "Expected a number, '(', or a sign here.");
    return ast_new_number(0);
}

static AstNode *parse_term(Parser *parser) {
    AstNode *left = parse_factor(parser);

    while (!parser->had_error &&
           (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) || check(parser, TOKEN_PERCENT))) {
        char op = parser->current.start[0];
        advance(parser);
        AstNode *right = parse_factor(parser);
        left = ast_new_binop(op, left, right);
    }

    return left;
}

static AstNode *parse_expr(Parser *parser) {
    AstNode *left = parse_term(parser);

    while (!parser->had_error && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        char op = parser->current.start[0];
        advance(parser);
        AstNode *right = parse_term(parser);
        left = ast_new_binop(op, left, right);
    }

    return left;
}

static int ident_is(Token token, const char *text) {
    size_t len = strlen(text);
    return (size_t)token.length == len && strncmp(token.start, text, len) == 0;
}

void parser_init(Parser *parser, Lexer *lexer, const char *source, const char *filename) {
    parser->lexer = lexer;
    parser->source = source;
    parser->filename = filename;
    parser->had_error = 0;
    advance(parser);
}

AstNode *parser_parse_program(Parser *parser) {
    if (!check(parser, TOKEN_IDENT) || !ident_is(parser->current, "vana")) {
        error_at_current(parser, "Expected 'vana' to start the program.");
        return ast_new_print(ast_new_number(0));
    }
    advance(parser);

    expect(parser, TOKEN_LPAREN, "Expected '(' after 'vana'.");
    AstNode *expr = parse_expr(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' to close 'vana(...)'.");
    expect(parser, TOKEN_EOF, "Unexpected extra input after 'vana(...)'.");

    return ast_new_print(expr);
}
