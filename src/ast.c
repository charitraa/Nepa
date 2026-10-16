#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

static AstNode *ast_alloc(AstType type) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "nepa: out of memory\n");
        exit(1);
    }
    node->type = type;
    return node;
}

AstNode *ast_new_number(long value) {
    AstNode *node = ast_alloc(AST_NUMBER);
    node->as.number.value = value;
    return node;
}

AstNode *ast_new_binop(char op, AstNode *left, AstNode *right) {
    AstNode *node = ast_alloc(AST_BINOP);
    node->as.binop.op = op;
    node->as.binop.left = left;
    node->as.binop.right = right;
    return node;
}

AstNode *ast_new_unary(char op, AstNode *operand) {
    AstNode *node = ast_alloc(AST_UNARY);
    node->as.unary.op = op;
    node->as.unary.operand = operand;
    return node;
}

AstNode *ast_new_print(AstNode *expr) {
    AstNode *node = ast_alloc(AST_PRINT);
    node->as.print.expr = expr;
    return node;
}

void ast_free(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER:
            break;
        case AST_BINOP:
            ast_free(node->as.binop.left);
            ast_free(node->as.binop.right);
            break;
        case AST_UNARY:
            ast_free(node->as.unary.operand);
            break;
        case AST_PRINT:
            ast_free(node->as.print.expr);
            break;
    }
    free(node);
}
