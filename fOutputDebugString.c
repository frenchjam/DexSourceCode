// Make it easier to construct messages and display them as a MessageBox.
#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>

#include "fOutputDebugString.h"
 
// Make it easier to construct messages to output in the debug window.
int fOutputDebugString( const char *format, ... ) {

	int items;

	va_list args;
	// The character buffers are really long so that there is little chance of overrunning.
	char message[10240];
	char fmt[10240];

	strcpy( fmt, "*** DEBUG OUTPUT: " );
	strcat( fmt, format );
	va_start(args, format);
	items = vsprintf(message, fmt, args);
	va_end(args);

	OutputDebugString( message );

	return( items );

}