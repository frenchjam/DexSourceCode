
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#include <fMessageBox.h>

int fMessageBox( int mb_type, const char *caption, const char *format, ... ) {
	
	int items;
	
	va_list args;
	char message[10240];
	
	va_start(args, format);
	items = vsprintf(message, format, args);
	va_end(args);
	
	return( MessageBox( NULL, message, caption, mb_type ) );
		
}
