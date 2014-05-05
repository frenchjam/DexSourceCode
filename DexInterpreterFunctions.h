#pragma once
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKENS	32

#ifdef __cplusplus
extern "C" {
#endif

int ParseCommaDelimitedLine ( char *tokens[MAX_TOKENS], const char *line );

#ifdef __cplusplus
}
#endif
