#ifndef NEPLANG_DIAGNOSTICS_H
#define NEPLANG_DIAGNOSTICS_H

#include "token.h"

void report_error(const char *source, const char *filename, Token token, const char *message);

#endif
