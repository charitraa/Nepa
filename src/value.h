#ifndef NEPLANG_VALUE_H
#define NEPLANG_VALUE_H

typedef enum {
    VAL_INT,
    VAL_ARRAY
} ValueType;

typedef struct Array Array;

typedef struct {
    ValueType type;
    union {
        long integer;
        Array *array;
    } as;
} Value;

/* Arrays are reference-counted and shared: assigning an array or passing it to a
 * function shares the same Array (like Python/Java), it does not copy it.
 * A cycle (an array that contains itself) is never freed; there is no cycle collector. */
struct Array {
    int refcount;
    int count;
    int capacity;
    Value *items;
};

/* Ownership convention: a function returning a Value hands the caller one reference.
 * Whoever holds a Value must eventually pass it to value_release(). */
Value value_int(long integer);
Value value_new_array(void);
void array_append(Array *array, Value item);  /* takes ownership of item */

Value value_retain(Value value);
void value_release(Value value);

const char *value_type_name(Value value);  /* "number" or "array", for error messages */
void value_print(Value value);

#endif
