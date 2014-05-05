
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include <fOutputDebugString.h>

int fOutputDebugString( const char *format, ... ) {

	int items;

	va_list args;
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
