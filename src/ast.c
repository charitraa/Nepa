#include <stdio.h>
#include <stdlib.h>
#include "ast.h"

static AstNode *ast_alloc(AstType type) {
    AstNode *node = malloc(sizeof(AstNode));
    if (!node) {
        fprintf(stderr, "neplang: out of memory\n");
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

AstNode *ast_new_binop(TokenType op, AstNode *left, AstNode *right) {
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

AstNode *ast_new_if(AstNode *condition, AstNode *then_block, AstNode *else_branch) {
    AstNode *node = ast_alloc(AST_IF);
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_block = then_block;
    node->as.if_stmt.else_branch = else_branch;
    return node;
}

AstNode *ast_new_while(AstNode *condition, AstNode *body) {
    AstNode *node = ast_alloc(AST_WHILE);
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

AstNode *ast_new_for(Token var, AstNode *start, AstNode *end, AstNode *body) {
    AstNode *node = ast_alloc(AST_FOR);
    node->as.for_stmt.var = var;
    node->as.for_stmt.start = start;
    node->as.for_stmt.end = end;
    node->as.for_stmt.body = body;
    return node;
}

AstNode *ast_new_block(void) {
    AstNode *node = ast_alloc(AST_BLOCK);
    node->as.block.statements = NULL;
    node->as.block.count = 0;
    node->as.block.capacity = 0;
    return node;
}

void ast_block_append(AstNode *block, AstNode *statement) {
    if (block->as.block.count == block->as.block.capacity) {
        int capacity = block->as.block.capacity == 0 ? 8 : block->as.block.capacity * 2;
        AstNode **statements = realloc(block->as.block.statements, sizeof(AstNode *) * (size_t)capacity);
        if (!statements) {
            fprintf(stderr, "neplang: out of memory\n");
            exit(1);
        }
        block->as.block.statements = statements;
        block->as.block.capacity = capacity;
    }
    block->as.block.statements[block->as.block.count++] = statement;
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
        case AST_IF:
            ast_free(node->as.if_stmt.condition);
            ast_free(node->as.if_stmt.then_block);
            ast_free(node->as.if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->as.while_stmt.condition);
            ast_free(node->as.while_stmt.body);
            break;
        case AST_FOR:
            ast_free(node->as.for_stmt.start);
            ast_free(node->as.for_stmt.end);
            ast_free(node->as.for_stmt.body);
            break;
        case AST_BLOCK:
            for (int i = 0; i < node->as.block.count; i++) {
                ast_free(node->as.block.statements[i]);
            }
            free(node->as.block.statements);
            break;
    }
    free(node);
}
