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

AstNode *ast_new_variable(Token name) {
    AstNode *node = ast_alloc(AST_VARIABLE);
    node->as.variable.name = name;
    return node;
}

AstNode *ast_new_assign(Token name, AstNode *value) {
    AstNode *node = ast_alloc(AST_ASSIGN);
    node->as.assign.name = name;
    node->as.assign.value = value;
    return node;
}

AstNode *ast_new_print(AstNode *expr) {
    AstNode *node = ast_alloc(AST_PRINT);
    node->as.print.expr = expr;
    return node;
}

AstNode *ast_new_program(void) {
    AstNode *node = ast_alloc(AST_PROGRAM);
    node->as.program.statements = NULL;
    node->as.program.count = 0;
    node->as.program.capacity = 0;
    return node;
}

void ast_program_append(AstNode *program, AstNode *statement) {
    if (program->as.program.count == program->as.program.capacity) {
        int capacity = program->as.program.capacity == 0 ? 8 : program->as.program.capacity * 2;
        AstNode **statements = realloc(program->as.program.statements, sizeof(AstNode *) * (size_t)capacity);
        if (!statements) {
            fprintf(stderr, "nepa: out of memory\n");
            exit(1);
        }
        program->as.program.statements = statements;
        program->as.program.capacity = capacity;
    }
    program->as.program.statements[program->as.program.count++] = statement;
}

void ast_free(AstNode *node) {
    if (!node) return;

    switch (node->type) {
        case AST_NUMBER:
        case AST_VARIABLE:
            break;
        case AST_BINOP:
            ast_free(node->as.binop.left);
            ast_free(node->as.binop.right);
            break;
        case AST_UNARY:
            ast_free(node->as.unary.operand);
            break;
        case AST_ASSIGN:
            ast_free(node->as.assign.value);
            break;
        case AST_PRINT:
            ast_free(node->as.print.expr);
            break;
        case AST_PROGRAM:
            for (int i = 0; i < node->as.program.count; i++) {
                ast_free(node->as.program.statements[i]);
            }
            free(node->as.program.statements);
            break;
    }
    free(node);
}
