#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DexInterpreterFunctions.h>

#define BUFFERS	16

#define iswhite( c ) ( c <= 0x20 || c >= 0x7f )

static char return_tokens[BUFFERS][1024];
static int circular = 0;

int ParseCommaDelimittedLine ( char *tokens[MAX_TOKENS], const char *line ) {


	char *tkn, *chr;
	int	 n = 0;

	if ( strlen( line ) > sizeof( return_tokens[circular] ) ) {
		fprintf( stderr, "Line too long.\n%s\n", line );
		exit( -1 );
	}
	strcpy( return_tokens[circular], line );
	
	tkn = strtok( return_tokens[circular], "," );

	if ( !strcmp( tkn, "CMD_SET_PICTURE" ) ) {
		n = n;
	}
	while ( tkn && n < MAX_TOKENS - 1 ) {
		/* Skip to first non-white character. */
		while ( iswhite( *tkn ) && *tkn ) tkn++;
		/* Strip off any trailing whitespace. */
		chr = tkn + strlen( tkn );
		while ( iswhite( *chr ) && chr >= tkn ) *chr-- = 0;
		/* Break out if we have hit a comment or the end of the line. */
		if ( *tkn == '#' || *tkn == 0 ) break;
		/* Record this as a valid token. */
		tokens[n++] = tkn;
		/* Parse for the next one. */
		tkn = strtok( NULL, "," );
	}

	/* Last token shall be a null pointer by definition. */
	tokens[n] = NULL;

	/* Next time around use a different buffer for the strings. */
	circular = ( circular + 1 ) % BUFFERS;

	return( n );
}

