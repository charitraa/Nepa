#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"

static long eval(AstNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            return node->as.number.value;

        case AST_UNARY: {
            long value = eval(node->as.unary.operand);
            return node->as.unary.op == '-' ? -value : value;
        }

        case AST_BINOP: {
            long left = eval(node->as.binop.left);
            long right = eval(node->as.binop.right);
            switch (node->as.binop.op) {
                case '+': return left + right;
                case '-': return left - right;
                case '*': return left * right;
                case '/':
                    if (right == 0) {
                        fprintf(stderr, "Runtime error: division by zero.\n");
                        exit(1);
                    }
                    return left / right;
                case '%':
                    if (right == 0) {
                        fprintf(stderr, "Runtime error: modulo by zero.\n");
                        exit(1);
                    }
                    return left % right;
            }
            break;
        }

        case AST_PRINT:
            fprintf(stderr, "Internal error: print node inside expression.\n");
            exit(1);
    }

    fprintf(stderr, "Internal error: unknown AST node.\n");
    exit(1);
}

void interpret(AstNode *program) {
    if (program->type != AST_PRINT) {
        fprintf(stderr, "Internal error: program root is not a print statement.\n");
        exit(1);
    }

    long result = eval(program->as.print.expr);
    printf("%ld\n", result);
}
