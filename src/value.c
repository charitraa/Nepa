#include <stdio.h>
#include <stdlib.h>
#include "value.h"

/* Deeply nested (or self-containing) arrays print as "[...]" past this depth. */
#define MAX_PRINT_DEPTH 32

Value value_int(long integer) {
    Value value;
    value.type = VAL_INT;
    value.as.integer = integer;
    return value;
}

Value value_new_array(void) {
    Array *array = malloc(sizeof(Array));
    if (!array) {
        fprintf(stderr, "neplang: out of memory\n");
        exit(1);
    }
    array->refcount = 1;
    array->count = 0;
    array->capacity = 0;
    array->items = NULL;

    Value value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    return value;
}

void array_append(Array *array, Value item) {
    if (array->count == array->capacity) {
        int capacity = array->capacity == 0 ? 8 : array->capacity * 2;
        Value *items = realloc(array->items, sizeof(Value) * (size_t)capacity);
        if (!items) {
            fprintf(stderr, "neplang: out of memory\n");
            exit(1);
        }
        array->items = items;
        array->capacity = capacity;
    }
    array->items[array->count++] = item;
}

Value value_retain(Value value) {
    if (value.type == VAL_ARRAY) {
        value.as.array->refcount++;
    }
    return value;
}

void value_release(Value value) {
    if (value.type != VAL_ARRAY) return;

    Array *array = value.as.array;
    if (--array->refcount > 0) return;

    for (int i = 0; i < array->count; i++) {
        value_release(array->items[i]);
    }
    free(array->items);
    free(array);
}

const char *value_type_name(Value value) {
    switch (value.type) {
        case VAL_INT:   return "number";
        case VAL_ARRAY: return "array";
    }
    return "value";
}

static void print_value(Value value, int depth) {
    switch (value.type) {
        case VAL_INT:
            printf("%ld", value.as.integer);
            break;

        case VAL_ARRAY:
            if (depth >= MAX_PRINT_DEPTH) {
                printf("[...]");
                break;
            }
            printf("[");
            for (int i = 0; i < value.as.array->count; i++) {
                if (i > 0) printf(", ");
                print_value(value.as.array->items[i], depth + 1);
            }
            printf("]");
            break;
    }
}

void value_print(Value value) {
    print_value(value, 0);
}
