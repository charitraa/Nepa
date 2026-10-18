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
            fprintf(stderr, "nepa: out of memory\n");
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

        case AST_ASSIGN:
        case AST_PRINT:
        case AST_PROGRAM:
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

        default:
            fprintf(stderr, "Internal error: expression node used as a statement.\n");
            exit(1);
    }
}

void interpret(AstNode *program, const char *source, const char *filename) {
    if (program->type != AST_PROGRAM) {
        fprintf(stderr, "Internal error: program root is not a program node.\n");
        exit(1);
    }

    Interpreter interp = { NULL, 0, 0, source, filename };
    for (int i = 0; i < program->as.program.count; i++) {
        execute(&interp, program->as.program.statements[i]);
    }
    free(interp.vars);
}
