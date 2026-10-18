#include "parser.h"
#include "diagnostics.h"

static AstNode *parse_expr(Parser *parser);

static void advance(Parser *parser) {
    parser->current = lexer_next_token(parser->lexer);
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static void error_at(Parser *parser, Token token, const char *message) {
    if (parser->had_error) return;
    parser->had_error = 1;
    report_error(parser->source, parser->filename, token, message);
}

static void error_at_current(Parser *parser, const char *message) {
    error_at(parser, parser->current, message);
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

    if (check(parser, TOKEN_IDENT)) {
        Token name = parser->current;
        advance(parser);
        return ast_new_variable(name);
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

    error_at_current(parser, "Expected a number, a variable, '(', or a sign here.");
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

static AstNode *parse_print(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (check(parser, TOKEN_EQUAL)) {
        error_at(parser, keyword, "'vana' is a reserved keyword and cannot be used as a variable name.");
        return ast_new_print(ast_new_number(0));
    }

    expect(parser, TOKEN_LPAREN, "Expected '(' after 'vana'.");
    AstNode *expr = parse_expr(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' to close 'vana(...)'.");
    return ast_new_print(expr);
}

static AstNode *parse_assignment(Parser *parser) {
    Token name = parser->current;
    advance(parser);

    if (!check(parser, TOKEN_EQUAL)) {
        error_at(parser, name,
                 "Unknown statement. Use 'vana(...)' to print, or 'name = value' to assign a variable.");
        return ast_new_assign(name, ast_new_number(0));
    }
    advance(parser);

    AstNode *value = parse_expr(parser);
    return ast_new_assign(name, value);
}

static AstNode *parse_statement(Parser *parser) {
    if (check(parser, TOKEN_VANA)) {
        return parse_print(parser);
    }
    if (check(parser, TOKEN_IDENT)) {
        return parse_assignment(parser);
    }

    error_at_current(parser, "Expected a statement here, such as 'vana(...)' or 'name = value'.");
    return ast_new_number(0);
}

static void skip_newlines(Parser *parser) {
    while (check(parser, TOKEN_NEWLINE)) {
        advance(parser);
    }
}

void parser_init(Parser *parser, Lexer *lexer, const char *source, const char *filename) {
    parser->lexer = lexer;
    parser->source = source;
    parser->filename = filename;
    parser->had_error = 0;
    advance(parser);
}

AstNode *parser_parse_program(Parser *parser) {
    AstNode *program = ast_new_program();

    skip_newlines(parser);
    while (!parser->had_error && !check(parser, TOKEN_EOF)) {
        ast_program_append(program, parse_statement(parser));

        if (!parser->had_error && !check(parser, TOKEN_NEWLINE) && !check(parser, TOKEN_EOF)) {
            error_at_current(parser, "Unexpected extra input. Each statement must be on its own line.");
        }
        skip_newlines(parser);
    }

    return program;
}
