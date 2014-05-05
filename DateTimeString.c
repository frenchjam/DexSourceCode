
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h> 
#include <windows.h>

#include <DateTimeString.h>

char *DateTimeString( void ) {

	struct tm *newtime;
	long ltime;

	// Use a circular buffer of string so that muliple calls don't write over previous results immediately.
	static char _datetimestr[64][64];
	static which_one = 0;

	which_one = ( which_one + 1 ) % 64;

	time( &ltime );
	newtime = localtime( &ltime );
	sprintf( _datetimestr[which_one], "%4d/%02d/%02d %02d:%02d:%02d", 
		newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday,
		newtime->tm_hour, newtime->tm_min, newtime->tm_sec );

	return( _datetimestr[which_one] );

}