#pragma once
#include <stdio.h>
#include <stdlib.h>

#define MAX_TOKENS	32

extern int ParseCommaDelimittedLine ( char *tokens[MAX_TOKENS], const char *line );
