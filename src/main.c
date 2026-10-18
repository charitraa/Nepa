#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "nepa: could not open file '%s'\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = malloc((size_t)size + 1);
    if (!buffer) {
        fprintf(stderr, "nepa: out of memory\n");
        exit(1);
    }

    size_t read = fread(buffer, 1, (size_t)size, file);
    buffer[read] = '\0';
    fclose(file);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: nepa <file.nepa>\n");
        return 1;
    }

    const char *filename = argv[1];
    char *source = read_file(filename);

    Lexer lexer;
    lexer_init(&lexer, source);

    Parser parser;
    parser_init(&parser, &lexer, source, filename);

    AstNode *program = parser_parse_program(&parser);

    if (parser.had_error) {
        ast_free(program);
        free(source);
        return 1;
    }

    interpret(program, source, filename);

    ast_free(program);
    free(source);
    return 0;
}
