#ifndef NEPLANG_AST_H
#define NEPLANG_AST_H

#include "token.h"

typedef enum {
    AST_NUMBER,
    AST_BINOP,
    AST_UNARY,
    AST_VARIABLE,
    AST_ASSIGN,
    AST_PRINT,
    AST_IF,
    AST_WHILE,
    AST_FOR,
    AST_BLOCK
} AstType;

typedef struct AstNode {
    AstType type;
    union {
        struct { long value; } number;
        struct { TokenType op; struct AstNode *left; struct AstNode *right; } binop;
        struct { char op; struct AstNode *operand; } unary;
        struct { Token name; } variable;
        struct { Token name; struct AstNode *value; } assign;
        struct { struct AstNode *expr; } print;
        struct { struct AstNode *condition; struct AstNode *then_block; struct AstNode *else_branch; } if_stmt;
        struct { struct AstNode *condition; struct AstNode *body; } while_stmt;
        struct { Token var; struct AstNode *start; struct AstNode *end; struct AstNode *body; } for_stmt;
        struct { struct AstNode **statements; int count; int capacity; } block;
    } as;
} AstNode;

AstNode *ast_new_number(long value);
AstNode *ast_new_binop(TokenType op, AstNode *left, AstNode *right);
AstNode *ast_new_unary(char op, AstNode *operand);
AstNode *ast_new_variable(Token name);
AstNode *ast_new_assign(Token name, AstNode *value);
AstNode *ast_new_print(AstNode *expr);
AstNode *ast_new_if(AstNode *condition, AstNode *then_block, AstNode *else_branch);
AstNode *ast_new_while(AstNode *condition, AstNode *body);
AstNode *ast_new_for(Token var, AstNode *start, AstNode *end, AstNode *body);
AstNode *ast_new_block(void);
void ast_block_append(AstNode *block, AstNode *statement);
void ast_free(AstNode *node);

#endif
