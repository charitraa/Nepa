#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "interpreter.h"
#include "diagnostics.h"
#include "value.h"

/* Deep enough for normal recursion, shallow enough to fail with a clear message
 * long before the C stack overflows. */
#define MAX_CALL_DEPTH 1000

/* Variables live in flat, linearly-searched scopes. Names are slices into the source buffer.
 * A real hashed symbol table arrives in the Symbol Table milestone.
 * Each Variable owns one reference to its value. */
typedef struct {
    const char *name;
    int length;
    Value value;
} Variable;

typedef struct {
    Variable *vars;
    int count;
    int capacity;
} Scope;

typedef struct {
    Scope globals;
    Scope *locals;          /* the current function call's scope; NULL at the top level */
    AstNode **functions;    /* AST_FUNCTION nodes, registered before execution starts */
    int function_count;
    int function_capacity;
    int call_depth;
    int returning;          /* set by 'farkau'; unwinds blocks and loops up to the call */
    Value return_value;     /* owned while `returning` is set */
    const char *source;
    const char *filename;
} Interpreter;

static void runtime_error(Interpreter *interp, Token token, const char *message) {
    if (token.start) {
        report_error(interp->source, interp->filename, token, message);
    } else {
        fprintf(stderr, "Runtime error: %s\n", message);
    }
    exit(1);
}

static int same_name(const char *name, int length, Token token) {
    return length == token.length && strncmp(name, token.start, (size_t)length) == 0;
}

static Variable *scope_find(Scope *scope, Token name) {
    for (int i = 0; i < scope->count; i++) {
        if (same_name(scope->vars[i].name, scope->vars[i].length, name)) {
            return &scope->vars[i];
        }
    }
    return NULL;
}

/* Takes ownership of `value`. */
static void scope_set(Scope *scope, Token name, Value value) {
    Variable *var = scope_find(scope, name);
    if (var) {
        value_release(var->value);
        var->value = value;
        return;
    }

    if (scope->count == scope->capacity) {
        int capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
        Variable *vars = realloc(scope->vars, sizeof(Variable) * (size_t)capacity);
        if (!vars) {
            fprintf(stderr, "neplang: out of memory\n");
            exit(1);
        }
        scope->vars = vars;
        scope->capacity = capacity;
    }

    var = &scope->vars[scope->count++];
    var->name = name.start;
    var->length = name.length;
    var->value = value;
}

static void scope_free(Scope *scope) {
    for (int i = 0; i < scope->count; i++) {
        value_release(scope->vars[i].value);
    }
    free(scope->vars);
}

/* Reads look in the current function's locals first, then in globals. */
static Variable *find_variable(Interpreter *interp, Token name) {
    if (interp->locals) {
        Variable *var = scope_find(interp->locals, name);
        if (var) return var;
    }
    return scope_find(&interp->globals, name);
}

/* Inside a function, assignment always creates/updates a local: a function can read
 * globals but never rebinds them. Takes ownership of `value`. */
static void set_variable(Interpreter *interp, Token name, Value value) {
    scope_set(interp->locals ? interp->locals : &interp->globals, name, value);
}

static int is_builtin(Token name) {
    return same_name("lambai", 6, name);
}

static AstNode *find_function(Interpreter *interp, Token name) {
    for (int i = 0; i < interp->function_count; i++) {
        Token fn = interp->functions[i]->as.function.name;
        if (same_name(fn.start, fn.length, name)) {
            return interp->functions[i];
        }
    }
    return NULL;
}

static void register_function(Interpreter *interp, AstNode *function) {
    Token name = function->as.function.name;
    char message[256];

    if (is_builtin(name)) {
        snprintf(message, sizeof(message), "'%.*s' is a built-in function and can't be redefined. Choose another name.",
                 name.length, name.start);
        runtime_error(interp, name, message);
    }

    AstNode *existing = find_function(interp, name);
    if (existing) {
        snprintf(message, sizeof(message), "Function '%.*s' is already defined on line %d. Give this one a different name.",
                 name.length, name.start, existing->as.function.name.line);
        runtime_error(interp, name, message);
    }

    if (interp->function_count == interp->function_capacity) {
        int capacity = interp->function_capacity == 0 ? 8 : interp->function_capacity * 2;
        AstNode **functions = realloc(interp->functions, sizeof(AstNode *) * (size_t)capacity);
        if (!functions) {
            fprintf(stderr, "neplang: out of memory\n");
            exit(1);
        }
        interp->functions = functions;
        interp->function_capacity = capacity;
    }
    interp->functions[interp->function_count++] = function;
}

static Value eval(Interpreter *interp, AstNode *node);
static void execute(Interpreter *interp, AstNode *node);

/* Evaluates `node` and requires a number. `what` completes "... must be a number". */
static long eval_int(Interpreter *interp, AstNode *node, const char *what) {
    Value value = eval(interp, node);
    if (value.type != VAL_INT) {
        char message[256];
        snprintf(message, sizeof(message), "%s must be a number, but this is an %s.", what, value_type_name(value));
        runtime_error(interp, node->token, message);
    }
    return value.as.integer;
}

static long eval_condition(Interpreter *interp, AstNode *node) {
    /* No boolean type yet: any non-zero number counts as true. */
    return eval_int(interp, node, "A condition") != 0;
}

/* Bounds-checks `index` against `array`; `at` is the '[' token. */
static void check_index(Interpreter *interp, Array *array, long index, Token at) {
    if (index >= 0 && index < array->count) return;

    char message[256];
    if (array->count == 0) {
        snprintf(message, sizeof(message), "Index %ld is out of range: this array is empty.", index);
    } else {
        snprintf(message, sizeof(message),
                 "Index %ld is out of range. This array has %d item%s, so valid indexes are 0 to %d.",
                 index, array->count, array->count == 1 ? "" : "s", array->count - 1);
    }
    runtime_error(interp, at, message);
}

/* Evaluates the array being indexed in `index_node` (an AST_INDEX); returns an owned reference. */
static Array *eval_indexed_array(Interpreter *interp, AstNode *index_node) {
    Value target = eval(interp, index_node->as.index.target);
    if (target.type != VAL_ARRAY) {
        char message[256];
        snprintf(message, sizeof(message), "Can't use [ ] on a %s. Only arrays can be indexed.", value_type_name(target));
        runtime_error(interp, index_node->token, message);
    }
    return target.as.array;
}

static Value call_builtin(Interpreter *interp, AstNode *call) {
    Token name = call->as.call.name;
    char message[256];

    /* lambai(array) -> number of items */
    if (call->as.call.arg_count != 1) {
        snprintf(message, sizeof(message), "'lambai' takes 1 argument (an array), but %d were given here.",
                 call->as.call.arg_count);
        runtime_error(interp, name, message);
    }
    AstNode *arg_node = call->as.call.args[0];
    Value arg = eval(interp, arg_node);
    if (arg.type != VAL_ARRAY) {
        snprintf(message, sizeof(message), "'lambai' needs an array, but this is a %s.", value_type_name(arg));
        runtime_error(interp, arg_node->token, message);
    }
    long length = arg.as.array->count;
    value_release(arg);
    return value_int(length);
}

static Value call_function(Interpreter *interp, AstNode *call) {
    Token name = call->as.call.name;
    char message[256];

    if (is_builtin(name)) {
        return call_builtin(interp, call);
    }

    AstNode *function = find_function(interp, name);
    if (!function) {
        snprintf(message, sizeof(message), "Function '%.*s' is not defined. Define it with 'kaam %.*s(...) { ... }'.",
                 name.length, name.start, name.length, name.start);
        runtime_error(interp, name, message);
    }

    int param_count = function->as.function.param_count;
    int arg_count = call->as.call.arg_count;
    if (arg_count != param_count) {
        snprintf(message, sizeof(message), "Function '%.*s' takes %d argument%s, but %d %s given here.",
                 name.length, name.start, param_count, param_count == 1 ? "" : "s",
                 arg_count, arg_count == 1 ? "was" : "were");
        runtime_error(interp, name, message);
    }

    if (interp->call_depth >= MAX_CALL_DEPTH) {
        snprintf(message, sizeof(message),
                 "Too many nested function calls (limit %d). Does '%.*s' call itself without ever stopping?",
                 MAX_CALL_DEPTH, name.length, name.start);
        runtime_error(interp, name, message);
    }

    /* Arguments are evaluated in the caller's scope, before the new scope exists. */
    Scope frame = { NULL, 0, 0 };
    for (int i = 0; i < arg_count; i++) {
        Value value = eval(interp, call->as.call.args[i]);
        scope_set(&frame, function->as.function.params[i], value);
    }

    Scope *saved_locals = interp->locals;
    interp->locals = &frame;
    interp->call_depth++;

    execute(interp, function->as.function.body);

    /* A function that ends without 'farkau' (or with a bare 'farkau') returns 0. */
    Value result = interp->returning ? interp->return_value : value_int(0);
    interp->returning = 0;
    interp->return_value = value_int(0);

    interp->call_depth--;
    interp->locals = saved_locals;
    scope_free(&frame);
    return result;
}

static Value eval_binop(Interpreter *interp, AstNode *node) {
    Value left = eval(interp, node->as.binop.left);
    Value right = eval(interp, node->as.binop.right);
    Token op = node->token;

    if (left.type != VAL_INT || right.type != VAL_INT) {
        char message[256];
        snprintf(message, sizeof(message), "Can't use '%.*s' with an array. It only works on numbers.",
                 op.length, op.start);
        runtime_error(interp, op, message);
    }

    long l = left.as.integer;
    long r = right.as.integer;
    switch (node->as.binop.op) {
        case TOKEN_PLUS:          return value_int(l + r);
        case TOKEN_MINUS:         return value_int(l - r);
        case TOKEN_STAR:          return value_int(l * r);
        case TOKEN_EQUAL_EQUAL:   return value_int(l == r);
        case TOKEN_BANG_EQUAL:    return value_int(l != r);
        case TOKEN_LESS:          return value_int(l < r);
        case TOKEN_LESS_EQUAL:    return value_int(l <= r);
        case TOKEN_GREATER:       return value_int(l > r);
        case TOKEN_GREATER_EQUAL: return value_int(l >= r);
        case TOKEN_SLASH:
            if (r == 0) runtime_error(interp, op, "Division by zero.");
            return value_int(l / r);
        case TOKEN_PERCENT:
            if (r == 0) runtime_error(interp, op, "Modulo by zero.");
            return value_int(l % r);
        default:
            break;
    }

    fprintf(stderr, "Internal error: unknown binary operator.\n");
    exit(1);
}

/* Returns an owned reference. */
static Value eval(Interpreter *interp, AstNode *node) {
    switch (node->type) {
        case AST_NUMBER:
            return value_int(node->as.number.value);

        case AST_VARIABLE: {
            Token name = node->as.variable.name;
            Variable *var = find_variable(interp, name);
            if (!var) {
                char message[256];
                snprintf(message, sizeof(message),
                         "Variable '%.*s' is not defined. Assign it a value first, e.g. '%.*s = 0'.",
                         name.length, name.start, name.length, name.start);
                runtime_error(interp, name, message);
            }
            return value_retain(var->value);
        }

        case AST_ARRAY: {
            Value array = value_new_array();
            for (int i = 0; i < node->as.array.count; i++) {
                array_append(array.as.array, eval(interp, node->as.array.items[i]));
            }
            return array;
        }

        case AST_INDEX: {
            Array *array = eval_indexed_array(interp, node);
            long index = eval_int(interp, node->as.index.index, "An array index");
            check_index(interp, array, index, node->token);

            /* Retain the item before releasing the array: the array may be a temporary. */
            Value item = value_retain(array->items[index]);
            value_release((Value){ .type = VAL_ARRAY, .as.array = array });
            return item;
        }

        case AST_CALL:
            return call_function(interp, node);

        case AST_UNARY: {
            long value = eval_int(interp, node->as.unary.operand, "The value after a sign");
            return value_int(node->as.unary.op == '-' ? -value : value);
        }

        case AST_BINOP:
            return eval_binop(interp, node);

        case AST_ASSIGN:
        case AST_INDEX_ASSIGN:
        case AST_PRINT:
        case AST_IF:
        case AST_WHILE:
        case AST_FOR:
        case AST_FOR_EACH:
        case AST_FUNCTION:
        case AST_RETURN:
        case AST_BLOCK:
            fprintf(stderr, "Internal error: statement node inside expression.\n");
            exit(1);
    }

    fprintf(stderr, "Internal error: unknown AST node.\n");
    exit(1);
}

static void execute(Interpreter *interp, AstNode *node) {
    switch (node->type) {
        case AST_PRINT: {
            Value value = eval(interp, node->as.print.expr);
            value_print(value);
            printf("\n");
            value_release(value);
            break;
        }

        case AST_ASSIGN:
            set_variable(interp, node->as.assign.name, eval(interp, node->as.assign.value));
            break;

        case AST_INDEX_ASSIGN: {
            AstNode *target = node->as.index_assign.target;
            Array *array = eval_indexed_array(interp, target);
            long index = eval_int(interp, target->as.index.index, "An array index");
            Value value = eval(interp, node->as.index_assign.value);
            check_index(interp, array, index, target->token);

            value_release(array->items[index]);
            array->items[index] = value;
            value_release((Value){ .type = VAL_ARRAY, .as.array = array });
            break;
        }

        case AST_CALL:
            value_release(eval(interp, node));  /* call used as a statement: result is discarded */
            break;

        case AST_IF:
            if (eval_condition(interp, node->as.if_stmt.condition)) {
                execute(interp, node->as.if_stmt.then_block);
            } else if (node->as.if_stmt.else_branch) {
                execute(interp, node->as.if_stmt.else_branch);
            }
            break;

        case AST_WHILE:
            while (!interp->returning && eval_condition(interp, node->as.while_stmt.condition)) {
                execute(interp, node->as.while_stmt.body);
            }
            break;

        case AST_FOR: {
            /* Bounds are evaluated once. The loop variable is reassigned from an internal
             * counter each iteration, so changing it inside the body doesn't skip iterations. */
            long start = eval_int(interp, node->as.for_stmt.start, "The start of a range");
            long end = eval_int(interp, node->as.for_stmt.end, "The end of a range");
            for (long i = start; i < end && !interp->returning; i++) {
                set_variable(interp, node->as.for_stmt.var, value_int(i));
                execute(interp, node->as.for_stmt.body);
            }
            break;
        }

        case AST_FOR_EACH: {
            AstNode *iterable = node->as.for_each.iterable;
            Value value = eval(interp, iterable);
            if (value.type != VAL_ARRAY) {
                runtime_error(interp, iterable->token,
                              "'ko_lagi ... in' needs an array or a range like '0..10', but this is a number.");
            }

            /* We hold a reference, so the array stays alive even if the body reassigns
             * the variable it came from. */
            Array *array = value.as.array;
            for (int i = 0; i < array->count && !interp->returning; i++) {
                set_variable(interp, node->as.for_each.var, value_retain(array->items[i]));
                execute(interp, node->as.for_each.body);
            }
            value_release(value);
            break;
        }

        case AST_FUNCTION:
            /* Already registered before execution started. */
            break;

        case AST_RETURN:
            interp->return_value = node->as.return_stmt.value ? eval(interp, node->as.return_stmt.value) : value_int(0);
            interp->returning = 1;
            break;

        case AST_BLOCK:
            for (int i = 0; i < node->as.block.count && !interp->returning; i++) {
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

    Interpreter interp;
    memset(&interp, 0, sizeof(interp));
    interp.return_value = value_int(0);
    interp.source = source;
    interp.filename = filename;

    /* Functions are registered up front, so a function can be called before
     * (above) its definition, and two functions can call each other. */
    for (int i = 0; i < program->as.block.count; i++) {
        if (program->as.block.statements[i]->type == AST_FUNCTION) {
            register_function(&interp, program->as.block.statements[i]);
        }
    }

    execute(&interp, program);

    scope_free(&interp.globals);
    free(interp.functions);
}
