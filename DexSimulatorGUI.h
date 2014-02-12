#pragma once

#include "DexApparatus.h"

extern HINSTANCE app_instance;
extern HWND	mouse_tracker_dlg;
extern HWND	status_dlg;
extern HWND	mass_dlg;
extern HWND	workspace_dlg;

BOOL CALLBACK dexDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
int IllustratedMessageBox( const char *picture, const char *message, const char *label, int buttons );
int fIllustratedMessageBox( int mb_type, const char *picture, const char *caption, const char *format, ... );

void DexInitPlots ( void );
void DexPlotData( DexApparatus *apparatus );

HWND DexInitGUI( HINSTANCE hInstance );
HWND DexCreateMouseTrackerGUI( void );
HWND DexCreateMassGUI( void );
void DexAddToLogGUI( const char *message );

void SaveGUIState( void );
void LoadGUIState( void );

#define TRACKER_UNALIGNED		0
#define TRACKER_ALIGNED_UPRIGHT	1
#define TRACKER_ALIGNED_SUPINE	2