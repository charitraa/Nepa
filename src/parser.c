#include <stdio.h>
#include "parser.h"
#include "diagnostics.h"

static AstNode *parse_comparison(Parser *parser);
static AstNode *parse_statement(Parser *parser);

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
        AstNode *inner = parse_comparison(parser);
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
        TokenType op = parser->current.type;
        advance(parser);
        AstNode *right = parse_factor(parser);
        left = ast_new_binop(op, left, right);
    }

    return left;
}

static AstNode *parse_expr(Parser *parser) {
    AstNode *left = parse_term(parser);

    while (!parser->had_error && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        TokenType op = parser->current.type;
        advance(parser);
        AstNode *right = parse_term(parser);
        left = ast_new_binop(op, left, right);
    }

    return left;
}

static int is_comparison(TokenType type) {
    return type == TOKEN_EQUAL_EQUAL || type == TOKEN_BANG_EQUAL ||
           type == TOKEN_LESS || type == TOKEN_LESS_EQUAL ||
           type == TOKEN_GREATER || type == TOKEN_GREATER_EQUAL;
}

/* Comparisons are non-associative: 'a < b < c' is rejected rather than
 * silently meaning '(a < b) < c'. */
static AstNode *parse_comparison(Parser *parser) {
    AstNode *left = parse_expr(parser);

    if (!parser->had_error && is_comparison(parser->current.type)) {
        TokenType op = parser->current.type;
        advance(parser);
        AstNode *right = parse_expr(parser);
        left = ast_new_binop(op, left, right);

        if (!parser->had_error && is_comparison(parser->current.type)) {
            error_at_current(parser,
                             "Comparisons can't be chained. Write each comparison inside its own 'yedi' instead.");
        }
    }

    return left;
}

/* Call right after consuming a keyword: catches 'yedi = 5' style misuse as a variable name. */
static int reject_keyword_as_name(Parser *parser, Token keyword) {
    if (!check(parser, TOKEN_EQUAL)) return 0;

    char message[128];
    snprintf(message, sizeof(message), "'%.*s' is a reserved keyword and cannot be used as a variable name.",
             keyword.length, keyword.start);
    error_at(parser, keyword, message);
    return 1;
}

static AstNode *parse_print(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_print(ast_new_number(0));
    }

    expect(parser, TOKEN_LPAREN, "Expected '(' after 'vana'.");
    AstNode *expr = parse_comparison(parser);
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

    AstNode *value = parse_comparison(parser);
    return ast_new_assign(name, value);
}

static void skip_newlines(Parser *parser) {
    while (check(parser, TOKEN_NEWLINE)) {
        advance(parser);
    }
}

/* Parses statements until `end` (TOKEN_RBRACE or TOKEN_EOF), leaving `end` as the current token.
 * A statement must be followed by a newline, or directly by the closing '}' of its block. */
static void parse_statements(Parser *parser, AstNode *block, TokenType end) {
    skip_newlines(parser);
    while (!parser->had_error && !check(parser, end) && !check(parser, TOKEN_EOF)) {
        ast_block_append(block, parse_statement(parser));

        if (!parser->had_error && !check(parser, TOKEN_NEWLINE) && !check(parser, end) &&
            !check(parser, TOKEN_EOF)) {
            error_at_current(parser, "Unexpected extra input. Each statement must be on its own line.");
        }
        skip_newlines(parser);
    }
}

static AstNode *parse_block(Parser *parser, const char *after) {
    AstNode *block = ast_new_block();
    if (!check(parser, TOKEN_LBRACE)) {
        char message[128];
        snprintf(message, sizeof(message), "Expected '{' to start the block after %s.", after);
        error_at_current(parser, message);
        return block;
    }

    Token open = parser->current;
    advance(parser);
    parse_statements(parser, block, TOKEN_RBRACE);

    if (!parser->had_error && !check(parser, TOKEN_RBRACE)) {
        error_at(parser, open, "This '{' is never closed. Add a matching '}' to end the block.");
        return block;
    }
    advance(parser);
    return block;
}

static AstNode *parse_condition(Parser *parser, Token keyword) {
    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_number(0);
    }
    if (check(parser, TOKEN_LBRACE)) {
        char message[128];
        snprintf(message, sizeof(message), "Expected a condition after '%.*s', e.g. '%.*s x > 0 {'.",
                 keyword.length, keyword.start, keyword.length, keyword.start);
        error_at_current(parser, message);
        return ast_new_number(0);
    }

    AstNode *condition = parse_comparison(parser);
    if (!parser->had_error && check(parser, TOKEN_EQUAL)) {
        error_at_current(parser, "Use '==' to compare values. A single '=' is only for assignment.");
    }
    return condition;
}

static AstNode *parse_if(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    AstNode *condition = parse_condition(parser, keyword);
    AstNode *then_block = parse_block(parser, "the 'yedi' condition");
    AstNode *else_branch = NULL;

    if (!parser->had_error && check(parser, TOKEN_NATARA)) {
        advance(parser);
        if (check(parser, TOKEN_YEDI)) {
            else_branch = parse_if(parser);
        } else {
            else_branch = parse_block(parser, "'natara'");
        }
    }

    return ast_new_if(condition, then_block, else_branch);
}

static AstNode *parse_while(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    AstNode *condition = parse_condition(parser, keyword);
    AstNode *body = parse_block(parser, "the 'jabasamma' condition");
    return ast_new_while(condition, body);
}

/* ko_lagi NAME in START..END { ... }  -- END is exclusive. */
static AstNode *parse_for(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_number(0);
    }

    Token var = expect(parser, TOKEN_IDENT, "Expected a loop variable name after 'ko_lagi', e.g. 'ko_lagi i in 0..10 {'.");
    expect(parser, TOKEN_IN, "Expected 'in' after the loop variable, e.g. 'ko_lagi i in 0..10 {'.");
    AstNode *start = parse_expr(parser);
    expect(parser, TOKEN_DOT_DOT, "Expected '..' between the start and end of the range, e.g. '0..10'.");
    AstNode *end = parse_expr(parser);
    AstNode *body = parse_block(parser, "the 'ko_lagi' range");
    return ast_new_for(var, start, end, body);
}

static AstNode *parse_statement(Parser *parser) {
    if (check(parser, TOKEN_VANA)) {
        return parse_print(parser);
    }
    if (check(parser, TOKEN_YEDI)) {
        return parse_if(parser);
    }
    if (check(parser, TOKEN_JABASAMMA)) {
        return parse_while(parser);
    }
    if (check(parser, TOKEN_KO_LAGI)) {
        return parse_for(parser);
    }
    if (check(parser, TOKEN_IN)) {
        Token keyword = parser->current;
        advance(parser);
        if (!reject_keyword_as_name(parser, keyword)) {
            error_at(parser, keyword, "'in' can only be used inside a loop, e.g. 'ko_lagi i in 0..10 {'.");
        }
        return ast_new_number(0);
    }
    if (check(parser, TOKEN_NATARA)) {
        error_at_current(parser,
                         "'natara' must come right after the closing '}' of a 'yedi' block, on the same line: '} natara {'.");
        return ast_new_number(0);
    }
    if (check(parser, TOKEN_RBRACE)) {
        error_at_current(parser, "Unexpected '}'. There is no open '{' block here to close.");
        return ast_new_number(0);
    }
    if (check(parser, TOKEN_IDENT)) {
        return parse_assignment(parser);
    }

    error_at_current(parser, "Expected a statement here, such as 'vana(...)' or 'name = value'.");
    return ast_new_number(0);
}

void parser_init(Parser *parser, Lexer *lexer, const char *source, const char *filename) {
    parser->lexer = lexer;
    parser->source = source;
    parser->filename = filename;
    parser->had_error = 0;
    advance(parser);
}

AstNode *parser_parse_program(Parser *parser) {
    AstNode *program = ast_new_block();
    parse_statements(parser, program, TOKEN_EOF);
    return program;
}
