#pragma once 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <windows.h>

#ifdef  __cplusplus
extern "C" {
#endif

int fMessageBox( int mb_type, const char *caption, const char *format, ... );

#ifdef  __cplusplus
}
#endif