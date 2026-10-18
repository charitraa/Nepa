#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "diagnostics.h"

/* Flat list of global variables. Names are slices into the source buffer.
 * A real hashed symbol table with scopes arrives in the Symbol Table milestone. */
typedef struct {
    const char *name;
    int length;
    long value;
} Variable;

typedef struct {
    Variable *vars;
    int count;
    int capacity;
    const char *source;
    const char *filename;
} Interpreter;

static Variable *find_variable(Interpreter *interp, Token name) {
    for (int i = 0; i < interp->count; i++) {
        Variable *var = &interp->vars[i];
        if (var->length == name.length && strncmp(var->name, name.start, (size_t)name.length) == 0) {
            return var;
        }
    }
    return NULL;
}

static void set_variable(Interpreter *interp, Token name, long value) {
    Variable *var = find_variable(interp, name);
    if (var) {
        var->value = value;
        return;
    }

    if (interp->count == interp->capacity) {
        int capacity = interp->capacity == 0 ? 8 : interp->capacity * 2;
        Variable *vars = realloc(interp->vars, sizeof(Variable) * (size_t)capacity);
        if (!vars) {
            fprintf(stderr, "neplang: out of memory\n");
            exit(1);
        }
        interp->vars = vars;
        interp->capacity = capacity;
    }

    var = &interp->vars[interp->count++];
    var->name = name.start;
    var->length = name.length;
    var->value = value;
}

static long eval(Interpreter *interp, AstNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            return node->as.number.value;

        case AST_VARIABLE: {
            Token name = node->as.variable.name;
            Variable *var = find_variable(interp, name);
            if (!var) {
                char message[256];
                snprintf(message, sizeof(message),
                         "Variable '%.*s' is not defined. Assign it a value first, e.g. '%.*s = 0'.",
                         name.length, name.start, name.length, name.start);
                report_error(interp->source, interp->filename, name, message);
                free(interp->vars);
                exit(1);
            }
            return var->value;
        }

        case AST_UNARY: {
            long value = eval(interp, node->as.unary.operand);
            return node->as.unary.op == '-' ? -value : value;
        }

        case AST_BINOP: {
            long left = eval(interp, node->as.binop.left);
            long right = eval(interp, node->as.binop.right);
            switch (node->as.binop.op) {
                case TOKEN_PLUS:          return left + right;
                case TOKEN_MINUS:         return left - right;
                case TOKEN_STAR:          return left * right;
                case TOKEN_EQUAL_EQUAL:   return left == right;
                case TOKEN_BANG_EQUAL:    return left != right;
                case TOKEN_LESS:          return left < right;
                case TOKEN_LESS_EQUAL:    return left <= right;
                case TOKEN_GREATER:       return left > right;
                case TOKEN_GREATER_EQUAL: return left >= right;
                case TOKEN_SLASH:
                    if (right == 0) {
                        fprintf(stderr, "Runtime error: division by zero.\n");
                        exit(1);
                    }
                    return left / right;
                case TOKEN_PERCENT:
                    if (right == 0) {
                        fprintf(stderr, "Runtime error: modulo by zero.\n");
                        exit(1);
                    }
                    return left % right;
                default:
                    break;
            }
            break;
        }

        case AST_ASSIGN:
        case AST_PRINT:
        case AST_IF:
        case AST_WHILE:
        case AST_FOR:
        case AST_BLOCK:
            fprintf(stderr, "Internal error: statement node inside expression.\n");
            exit(1);
    }

    fprintf(stderr, "Internal error: unknown AST node.\n");
    exit(1);
}

static void execute(Interpreter *interp, AstNode *node) {
    switch (node->type) {
        case AST_PRINT:
            printf("%ld\n", eval(interp, node->as.print.expr));
            break;

        case AST_ASSIGN:
            set_variable(interp, node->as.assign.name, eval(interp, node->as.assign.value));
            break;

        case AST_IF:
            /* No boolean type yet: any non-zero value counts as true. */
            if (eval(interp, node->as.if_stmt.condition) != 0) {
                execute(interp, node->as.if_stmt.then_block);
            } else if (node->as.if_stmt.else_branch) {
                execute(interp, node->as.if_stmt.else_branch);
            }
            break;

        case AST_WHILE:
            while (eval(interp, node->as.while_stmt.condition) != 0) {
                execute(interp, node->as.while_stmt.body);
            }
            break;

        case AST_FOR: {
            /* Bounds are evaluated once. The loop variable is reassigned from an internal
             * counter each iteration, so changing it inside the body doesn't skip iterations. */
            long start = eval(interp, node->as.for_stmt.start);
            long end = eval(interp, node->as.for_stmt.end);
            for (long i = start; i < end; i++) {
                set_variable(interp, node->as.for_stmt.var, i);
                execute(interp, node->as.for_stmt.body);
            }
            break;
        }

        case AST_BLOCK:
            for (int i = 0; i < node->as.block.count; i++) {
                execute(interp, node->as.block.statements[i]);
            }
            break;

        default:
            fprintf(stderr, "Internal error: expression node used as a statement.\n");
            exit(1);
    }
}

void interpret(AstNode *program, const char *source, const char *filename) {
    if (program->type != AST_BLOCK) {
        fprintf(stderr, "Internal error: program root is not a block node.\n");
        exit(1);
    }

    Interpreter interp = { NULL, 0, 0, source, filename };
    execute(&interp, program);
    free(interp.vars);
}
