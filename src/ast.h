#ifndef NEPA_AST_H
#define NEPA_AST_H

typedef enum {
    AST_NUMBER,
    AST_BINOP,
    AST_UNARY,
    AST_PRINT
} AstType;

typedef struct AstNode {
    AstType type;
    union {
        struct { long value; } number;
        struct { char op; struct AstNode *left; struct AstNode *right; } binop;
        struct { char op; struct AstNode *operand; } unary;
        struct { struct AstNode *expr; } print;
    } as;
} AstNode;

AstNode *ast_new_number(long value);
AstNode *ast_new_binop(char op, AstNode *left, AstNode *right);
AstNode *ast_new_unary(char op, AstNode *operand);
AstNode *ast_new_print(AstNode *expr);
void ast_free(AstNode *node);

#endif
