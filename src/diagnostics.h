#ifndef NEPA_DIAGNOSTICS_H
#define NEPA_DIAGNOSTICS_H

#include "token.h"

void report_error(const char *source, const char *filename, Token token, const char *message);

#endif
