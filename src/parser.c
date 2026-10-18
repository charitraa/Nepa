#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "diagnostics.h"

static AstNode *parse_comparison(Parser *parser);
static AstNode *parse_statement(Parser *parser);
static void skip_newlines(Parser *parser);

static void error_at(Parser *parser, Token token, const char *message);

/* Lexer errors are reported as soon as the bad character is scanned, so the message
 * names the character instead of whatever the parser expected in its place. */
static void advance(Parser *parser) {
    parser->current = lexer_next_token(parser->lexer);

    Token token = parser->current;
    if (token.type == TOKEN_ERROR) {
        char message[256];
        if ((unsigned char)token.start[0] >= 0x80) {
            snprintf(message, sizeof(message),
                     "Unexpected character '%.*s'. Names can only use English letters, digits, and '_'.",
                     token.length, token.start);
        } else {
            snprintf(message, sizeof(message), "Unexpected character '%.*s'.", token.length, token.start);
        }
        error_at(parser, token, message);
    }
}

static int check(Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser *parser, TokenType type) {
    if (!check(parser, type)) return 0;
    advance(parser);
    return 1;
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

static int same_name(Token a, Token b) {
    return a.length == b.length && strncmp(a.start, b.start, (size_t)a.length) == 0;
}

/* Called with the function name consumed and '(' as the current token. */
static AstNode *parse_call(Parser *parser, Token name) {
    AstNode *call = ast_new_call(name);
    advance(parser);

    if (!check(parser, TOKEN_RPAREN)) {
        do {
            ast_call_add_arg(call, parse_comparison(parser));
        } while (!parser->had_error && match(parser, TOKEN_COMMA));
    }

    expect(parser, TOKEN_RPAREN, "Expected ',' or ')' in the function call's argument list.");
    return call;
}

/* [a, b, c] -- newlines are allowed between items so long arrays can span lines. */
static AstNode *parse_array(Parser *parser) {
    Token open = parser->current;
    AstNode *array = ast_new_array(open);
    advance(parser);
    skip_newlines(parser);

    if (!check(parser, TOKEN_RBRACKET)) {
        do {
            skip_newlines(parser);
            ast_array_add_item(array, parse_comparison(parser));
            skip_newlines(parser);
        } while (!parser->had_error && match(parser, TOKEN_COMMA));
    }

    if (!parser->had_error && !check(parser, TOKEN_RBRACKET) && parser->current.line != open.line) {
        error_at(parser, open, "This '[' is never closed. Add a matching ']' to end the array.");
    }
    expect(parser, TOKEN_RBRACKET, "Expected ',' or ']' in the array.");
    return array;
}

/* Any number of trailing '[index]' after a value: a[i], m[i][j], f(x)[0]. */
static AstNode *parse_postfix(Parser *parser, AstNode *node) {
    while (!parser->had_error && check(parser, TOKEN_LBRACKET)) {
        Token open = parser->current;
        advance(parser);
        AstNode *index = parse_comparison(parser);
        expect(parser, TOKEN_RBRACKET, "Expected ']' to close the index.");
        node = ast_new_index(open, node, index);
    }
    return node;
}

static AstNode *parse_primary(Parser *parser) {
    if (check(parser, TOKEN_INT)) {
        Token literal = parser->current;
        advance(parser);
        return ast_new_number(literal);
    }

    if (check(parser, TOKEN_IDENT)) {
        Token name = parser->current;
        advance(parser);
        if (check(parser, TOKEN_LPAREN)) {
            return parse_call(parser, name);
        }
        return ast_new_variable(name);
    }

    if (check(parser, TOKEN_LBRACKET)) {
        return parse_array(parser);
    }

    if (check(parser, TOKEN_LPAREN)) {
        advance(parser);
        AstNode *inner = parse_comparison(parser);
        expect(parser, TOKEN_RPAREN, "Expected ')' to close the expression.");
        return inner;
    }

    error_at_current(parser, "Expected a number, a variable, '(', '[', or a sign here.");
    return ast_new_placeholder();
}

static AstNode *parse_factor(Parser *parser) {
    if (parser->had_error) return ast_new_placeholder();

    if (check(parser, TOKEN_MINUS) || check(parser, TOKEN_PLUS)) {
        Token op = parser->current;
        advance(parser);
        AstNode *operand = parse_factor(parser);
        return ast_new_unary(op, operand);
    }

    return parse_postfix(parser, parse_primary(parser));
}

static AstNode *parse_term(Parser *parser) {
    AstNode *left = parse_factor(parser);

    while (!parser->had_error &&
           (check(parser, TOKEN_STAR) || check(parser, TOKEN_SLASH) || check(parser, TOKEN_PERCENT))) {
        Token op = parser->current;
        advance(parser);
        AstNode *right = parse_factor(parser);
        left = ast_new_binop(op, left, right);
    }

    return left;
}

static AstNode *parse_expr(Parser *parser) {
    AstNode *left = parse_term(parser);

    while (!parser->had_error && (check(parser, TOKEN_PLUS) || check(parser, TOKEN_MINUS))) {
        Token op = parser->current;
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
        Token op = parser->current;
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
        return ast_new_print(ast_new_placeholder());
    }

    expect(parser, TOKEN_LPAREN, "Expected '(' after 'vana'.");
    AstNode *expr = parse_comparison(parser);
    expect(parser, TOKEN_RPAREN, "Expected ')' to close 'vana(...)'.");
    return ast_new_print(expr);
}

static AstNode *parse_assignment(Parser *parser) {
    Token name = parser->current;
    advance(parser);

    if (check(parser, TOKEN_LPAREN)) {
        return parse_call(parser, name);
    }

    /* name[i]... = value */
    AstNode *target = NULL;
    if (check(parser, TOKEN_LBRACKET)) {
        target = parse_postfix(parser, ast_new_variable(name));
    }

    if (!check(parser, TOKEN_EQUAL)) {
        error_at(parser, name,
                 "Unknown statement. Use 'vana(...)' to print, 'name = value' to assign, or 'name(...)' to call a function.");
        ast_free(target);
        return ast_new_placeholder();
    }
    advance(parser);

    AstNode *value = parse_comparison(parser);
    if (target) {
        return ast_new_index_assign(target, value);
    }
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
    parser->block_depth++;
    parse_statements(parser, block, TOKEN_RBRACE);
    parser->block_depth--;

    if (!parser->had_error && !check(parser, TOKEN_RBRACE)) {
        error_at(parser, open, "This '{' is never closed. Add a matching '}' to end the block.");
        return block;
    }
    advance(parser);
    return block;
}

static AstNode *parse_condition(Parser *parser, Token keyword) {
    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_placeholder();
    }
    if (check(parser, TOKEN_LBRACE)) {
        char message[128];
        snprintf(message, sizeof(message), "Expected a condition after '%.*s', e.g. '%.*s x > 0 {'.",
                 keyword.length, keyword.start, keyword.length, keyword.start);
        error_at_current(parser, message);
        return ast_new_placeholder();
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

/* ko_lagi NAME in START..END { ... }  -- END is exclusive.
 * ko_lagi NAME in ARRAY { ... }       -- one iteration per element. */
static AstNode *parse_for(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_placeholder();
    }

    Token var = expect(parser, TOKEN_IDENT, "Expected a loop variable name after 'ko_lagi', e.g. 'ko_lagi i in 0..10 {'.");
    expect(parser, TOKEN_IN, "Expected 'in' after the loop variable, e.g. 'ko_lagi i in 0..10 {'.");
    AstNode *start = parse_expr(parser);

    if (!parser->had_error && !check(parser, TOKEN_DOT_DOT)) {
        if (!check(parser, TOKEN_LBRACE)) {
            error_at_current(parser, "Expected '..' for a range like '0..10', or '{' to loop over an array.");
        }
        AstNode *body = parse_block(parser, "the 'ko_lagi' array");
        return ast_new_for_each(var, start, body);
    }

    expect(parser, TOKEN_DOT_DOT, "Expected '..' between the start and end of the range, e.g. '0..10'.");
    AstNode *end = parse_expr(parser);
    AstNode *body = parse_block(parser, "the 'ko_lagi' range");
    return ast_new_for(var, start, end, body);
}

/* kaam NAME(PARAM, ...) { ... }  -- only allowed at the top level of a file. */
static AstNode *parse_function(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_placeholder();
    }
    if (parser->block_depth > 0) {
        error_at(parser, keyword,
                 "Functions must be defined at the top level of the file, not inside a block or another function.");
        return ast_new_placeholder();
    }

    Token name = expect(parser, TOKEN_IDENT, "Expected a function name after 'kaam', e.g. 'kaam jamma(a, b) {'.");
    AstNode *function = ast_new_function(name);
    expect(parser, TOKEN_LPAREN, "Expected '(' after the function name, e.g. 'kaam jamma(a, b) {'.");

    if (!parser->had_error && !check(parser, TOKEN_RPAREN)) {
        do {
            Token param = expect(parser, TOKEN_IDENT, "Expected a parameter name here.");
            if (parser->had_error) break;

            for (int i = 0; i < function->as.function.param_count; i++) {
                if (same_name(function->as.function.params[i], param)) {
                    char message[256];
                    snprintf(message, sizeof(message), "Parameter '%.*s' is listed twice. Each parameter needs a different name.",
                             param.length, param.start);
                    error_at(parser, param, message);
                    break;
                }
            }
            ast_function_add_param(function, param);
        } while (!parser->had_error && match(parser, TOKEN_COMMA));
    }
    expect(parser, TOKEN_RPAREN, "Expected ',' or ')' in the parameter list.");

    parser->in_function = 1;
    function->as.function.body = parse_block(parser, "the function's parameter list");
    parser->in_function = 0;
    return function;
}

static AstNode *parse_return(Parser *parser) {
    Token keyword = parser->current;
    advance(parser);

    if (reject_keyword_as_name(parser, keyword)) {
        return ast_new_placeholder();
    }
    if (!parser->in_function) {
        error_at(parser, keyword, "'farkau' can only be used inside a function ('kaam').");
        return ast_new_placeholder();
    }

    AstNode *value = NULL;
    if (!check(parser, TOKEN_NEWLINE) && !check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        value = parse_comparison(parser);
    }
    return ast_new_return(keyword, value);
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
    if (check(parser, TOKEN_KAAM)) {
        return parse_function(parser);
    }
    if (check(parser, TOKEN_FARKAU)) {
        return parse_return(parser);
    }
    if (check(parser, TOKEN_IN)) {
        Token keyword = parser->current;
        advance(parser);
        if (!reject_keyword_as_name(parser, keyword)) {
            error_at(parser, keyword, "'in' can only be used inside a loop, e.g. 'ko_lagi i in 0..10 {'.");
        }
        return ast_new_placeholder();
    }
    if (check(parser, TOKEN_NATARA)) {
        error_at_current(parser,
                         "'natara' must come right after the closing '}' of a 'yedi' block, on the same line: '} natara {'.");
        return ast_new_placeholder();
    }
    if (check(parser, TOKEN_RBRACE)) {
        error_at_current(parser, "Unexpected '}'. There is no open '{' block here to close.");
        return ast_new_placeholder();
    }
    if (check(parser, TOKEN_IDENT)) {
        return parse_assignment(parser);
    }

    error_at_current(parser, "Expected a statement here, such as 'vana(...)' or 'name = value'.");
    return ast_new_placeholder();
}

void parser_init(Parser *parser, Lexer *lexer, const char *source, const char *filename) {
    parser->lexer = lexer;
    parser->source = source;
    parser->filename = filename;
    parser->had_error = 0;
    parser->block_depth = 0;
    parser->in_function = 0;
    advance(parser);
}

AstNode *parser_parse_program(Parser *parser) {
    AstNode *program = ast_new_block();
    parse_statements(parser, program, TOKEN_EOF);
    return program;
}
