#include <stdio.h>
#include "diagnostics.h"

void report_error(const char *source, const char *filename, Token token, const char *message) {
    const char *line_start = token.start;
    while (line_start > source && line_start[-1] != '\n') {
        line_start--;
    }

    const char *line_end = token.start;
    while (*line_end != '\n' && *line_end != '\0') {
        line_end++;
    }

    fprintf(stderr, "Error in %s at line %d, column %d:\n\n", filename, token.line, token.col);
    fprintf(stderr, "    %.*s\n", (int)(line_end - line_start), line_start);
    fprintf(stderr, "    %*s^\n\n", token.col - 1, "");
    fprintf(stderr, "%s\n", message);
}
