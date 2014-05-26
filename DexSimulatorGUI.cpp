/*********************************************************************************/
/*                                                                               */
/*                             DexSimulatorApp.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Demonstrate the principle steps that need to be accomplished in a DEX protocol.
 * Demonstrate the concept of a script compiler and interpreter.
 * Demonstrate how to communicate with the CODA.
 */

/*********************************************************************************/

#include "..\DexSimulatorApp\resource.h"

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"
#include "DexSimulatorGUI.h"

#include <3dMatrix.h>
#include <OglDisplayInterface.h>
#include <Views.h>
#include <Layouts.h>


/*********************************************************************************/

// Windows dialogs and stuff.

HINSTANCE app_instance;
HWND	mouse_tracker_dlg;
HWND	status_dlg;
HWND	mass_dlg;
HWND	camera_dlg;
HWND	workspace_dlg;

// Holds the text of the log messages.
static char _dexLog[102400] = "";
static int  _dexLogNext = 0;

// Refresh rate for the log display.
static DexTimer _refresh_timer;
static double _refresh_rate = 2.0;

// 2D Graphics
Display display; 
Layout  layout;
View	view, yz_view, cop_view;

int plot_screen_left = 225;
int plot_screen_top = 10;
int plot_screen_width = 772;
int plot_screen_height = 600;
float _plot_z_min = -500.0;
float _plot_z_max =  500.0;
float _plot_y_min = -200.0;
float _plot_y_max =  800.0;
int   _n_plots = 7;

float cop_range = 0.03;
float load_range = 100.0;
float grip_range = 25.0;

double Vt[DEX_MAX_MARKER_FRAMES];
char *axis_name[] = { "X", "Y", "Z", "M" };
char PictureFilenamePrefix[] = "..\\DexPictures\\";

/**************************************************************************************/

void DexInitPlots ( void ) {


	display = DefaultDisplay();
	DisplaySetSizePixels( display, plot_screen_width, plot_screen_height );
	DisplaySetScreenPosition( display, plot_screen_left, plot_screen_top );
	DisplaySetName( display, "DEX Simulator - Recorded Data" );
	DisplayInit( display );
	Erase( display );
	
	yz_view = CreateView( display );
	ViewSetDisplayEdgesRelative( yz_view, 0.01, 0.51, 0.30, 0.99 );
	ViewSetEdges( yz_view, _plot_z_min, _plot_y_min, _plot_z_max, _plot_y_max );
	ViewMakeSquare( yz_view );
	
	cop_view = CreateView( display );
	ViewSetDisplayEdgesRelative( cop_view, 0.01, 0.01, 0.30, 0.49 );
	ViewSetEdges( cop_view, - cop_range, - cop_range, cop_range, cop_range );
	ViewMakeSquare( cop_view );

	layout = CreateLayout( display, _n_plots, 1 );
	LayoutSetDisplayEdgesRelative( layout, 0.31, 0.01, 0.99, 0.99 );

	ActivateDisplayWindow();
	Erase( display );
	DisplaySwap( display );
	HideDisplayWindow();

}

void DexPlotData( DexApparatus *apparatus ) {

	int i, j, cnt;
	Vector3 delta;
	double max_time;
	double filtered, filter_constant = 1.0;

	if ( apparatus->nAcqFrames < 1 ) return;

	// Compute velocity.
	for ( i = 0; i < apparatus->nAcqFrames - 1; i++ ) {
		apparatus->SubtractVectors( delta, apparatus->acquiredManipulandumState[i+1].position, apparatus->acquiredManipulandumState[i].position );
		apparatus->ScaleVector( delta, delta, 1.0 / ( apparatus->acquiredManipulandumState[i+1].time - apparatus->acquiredManipulandumState[i].time ) );
		Vt[i] = apparatus->VectorNorm( delta );
	}
	// Back and forth filter to avoid phase lag.
	for ( i = 0, filtered = 0.0; i < apparatus->nAcqFrames - 1; i++ ) {
		filtered = (filter_constant * filtered + Vt[i]) / (1.0 + filter_constant);
		Vt[i] = filtered;
	}
	for ( i = apparatus->nAcqFrames - 2, filtered = 0.0; i >=0 ; i-- ) {
		filtered = (filter_constant * filtered + Vt[i]) / (1.0 + filter_constant);
		Vt[i] = filtered;
	}
	// There is one less sample, due to the need for the finite difference.
	Vt[apparatus->nAcqFrames - 1] = INVISIBLE;

	max_time = apparatus->acquiredManipulandumState[apparatus->nAcqFrames-1].time;

	ShowDisplayWindow();
	ActivateDisplayWindow();
	Erase( display );
		
	ViewColor( yz_view, GREY4 );
	ViewBox( yz_view );
	ViewTitle( yz_view, "YZ", INSIDE_LEFT, INSIDE_TOP, 0.0 );
	ViewBox( cop_view );
	ViewCircle( cop_view, 0.0, 0.0, copTolerance / 1000.0 );
	ViewTitle( cop_view, "CoP", INSIDE_LEFT, INSIDE_TOP, 0.0 );
	
	int frames = apparatus->nAcqFrames;
	int samples = apparatus->nAcqSamples;
	if ( frames > 0 ) {

		ViewColor( yz_view, RED );
		ViewPenSize( yz_view, 5 );
		ViewXYPlotAvailableDoubles( yz_view,
			&apparatus->acquiredManipulandumState[0].position[Z], 
			&apparatus->acquiredManipulandumState[0].position[Y], 
			0, frames - 1, 
			sizeof( *apparatus->acquiredManipulandumState ), 
			sizeof( *apparatus->acquiredManipulandumState ), 
			INVISIBLE );

		for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
			ViewSelectColor( cop_view, i );
			ViewPenSize( cop_view, 5 );
			ViewXYPlotAvailableDoubles( cop_view,
				&apparatus->acquiredCOP[i][0][X], 
				&apparatus->acquiredCOP[i][0][Y], 
				0, samples - 1, 
				sizeof( *apparatus->acquiredCOP[i] ), 
				sizeof( *apparatus->acquiredCOP[i] ), 
				INVISIBLE );
		}

		for ( i = 0, cnt = 0; i < 3; i++, cnt++ ) {
		
			view = LayoutViewN( layout, cnt );
			ViewColor( view, GREY4 );	
			ViewBox( view );
			ViewTitle( view, axis_name[cnt], INSIDE_LEFT, INSIDE_TOP, 0.0 );
		
			ViewSetXLimits( view, 0.0, max_time );
			ViewAutoScaleInit( view );
			ViewAutoScaleAvailableDoubles( view,
				&apparatus->acquiredManipulandumState[0].position[i], 
				0, frames - 1, 
				sizeof( *apparatus->acquiredManipulandumState ), 
				INVISIBLE );
			ViewAutoScaleSetInterval( view, 500.0 );			
			
			ViewSelectColor( view, i );
			ViewXYPlotAvailableDoubles( view,
				&apparatus->acquiredManipulandumState[0].time, 
				&apparatus->acquiredManipulandumState[0].position[i], 
				0, frames - 1, 
				sizeof( *apparatus->acquiredManipulandumState ), 
				sizeof( *apparatus->acquiredManipulandumState ), 
				INVISIBLE );
			
		}

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewTitle( view, "Vt", INSIDE_LEFT, INSIDE_TOP, 0.0 );
		ViewBox( view );
		// Set the span to be covered by autoscale.
		ViewSetXLimits( view, 0.0, frames );
		ViewAutoScaleInit( view );
		ViewAutoScaleAvailableDoubles( view, 
			Vt, 0, frames - 1, 
			sizeof( *Vt ), 
			INVISIBLE );		
		ViewColor( view, BLUE );
		// Now set the span in terms of time.
		ViewSetXLimits( view, 0.0, max_time );
		ViewXYPlotAvailableDoubles( view, 
			&apparatus->acquiredManipulandumState[0].time, 
			Vt, 
			0, frames - 1, 
			sizeof( *apparatus->acquiredManipulandumState ), 
			sizeof( *Vt ), 
			INVISIBLE );

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
		ViewTitle( view, "GF", INSIDE_LEFT, INSIDE_TOP, 0.0 );
	
		ViewSetXLimits( view, 0.0, samples );
		ViewAutoScaleInit( view );
		ViewAutoScaleAvailableDoubles( view,
			&apparatus->acquiredGripForce[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredGripForce ), 
			INVISIBLE );
		ViewAutoScaleSetInterval( view, grip_range );			
		ViewSetXLimits( view, 0.0, max_time );
		
		ViewColor( view, GREY4 );	
		ViewLine( view, 0.0, 0.0, max_time, 0.0 );

		ViewColor( view, RED );
		ViewXYPlotAvailableDoubles( view,
			&apparatus->acquiredAnalog[0].time, 
			&apparatus->acquiredGripForce[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredAnalog ), 
			sizeof( *apparatus->acquiredGripForce ), 
			INVISIBLE );

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
		ViewTitle( view, "LF", INSIDE_LEFT, INSIDE_TOP, 0.0 );
	
		ViewSetXLimits( view, 0.0, samples );
		ViewAutoScaleInit( view );
		ViewAutoScaleAvailableDoubles( view,
			&apparatus->acquiredLoadForceMagnitude[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredLoadForceMagnitude ), 
			INVISIBLE );
		ViewAutoScaleSetInterval( view, load_range );			
		ViewSetXLimits( view, 0.0, max_time );
		
		ViewColor( view, BLUE );
		ViewXYPlotAvailableDoubles( view,
			&apparatus->acquiredAnalog[0].time, 
			&apparatus->acquiredLoadForceMagnitude[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredAnalog ), 
			sizeof( *apparatus->acquiredLoadForceMagnitude ), 
			INVISIBLE );

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
		ViewTitle( view, "COP", INSIDE_LEFT, INSIDE_TOP, 0.0 );
	
		ViewSetXLimits( view, 0.0, max_time );
		ViewSetYLimits( view, - 2.0 * copTolerance / 1000.0, 2.0 * copTolerance / 1000.0 );
		ViewLine( view, 0.0,   copTolerance / 1000.0, max_time,   copTolerance / 1000.0 );
		ViewLine( view, 0.0, - copTolerance / 1000.0, max_time, - copTolerance / 1000.0 );
		for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
			for ( j = 0; j < 2; j++ ) {
				ViewSelectColor( view, i * N_FORCE_TRANSDUCERS + j );
					ViewXYPlotAvailableDoubles( view,
						&apparatus->acquiredAnalog[0].time, 
						&apparatus->acquiredCOP[i][0][j], 
						0, samples - 1, 
						sizeof( *apparatus->acquiredAnalog ), 
						sizeof( *apparatus->acquiredCOP[i] ), 
						INVISIBLE );
			}
		}

		// Plot the events.
		for ( i = 0; i < apparatus->nEvents; i++ ) {
			double size;
			// There are a lot of target and sound events, so we plot them as tick marks.
			// Other events are plotted as full lines.
			// If 2 events coincide, you will probably only see one of them.
			if ( apparatus->eventList[i].event == TARGET_EVENT || apparatus->eventList[i].event == SOUND_EVENT ) size = 0.1;
			else size = 1.0;
			for ( j = 0; j < cnt; j++ ) {
				view = LayoutViewN( layout, j );
				double bottom = view->user_bottom;
				double top = bottom + size * (view->user_top - view->user_bottom);
				ViewSelectColor( view, apparatus->eventList[i].event );	
				ViewLine( view, apparatus->eventList[i].time, bottom, apparatus->eventList[i].time, top );
			}
		}
		
	}
	
	DisplaySwap( display );
	
	RunWindow();
	HideDisplayWindow();
	
	
}

/*********************************************************************************/

// Mesage handler for dialog boxes. Closing these boxes closes the APP.

BOOL CALLBACK dexDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	int inA, inB, inC, onM;

	switch (message)
	{
	case WM_INITDIALOG:
        
//		SetTimer( hDlg, 1, 1000, NULL );
		return TRUE;
		break;
		
    case WM_PAINT:
		return FALSE;
		break;

    case WM_TIMER:
		return TRUE;
		break;
		
    case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		// Closing any of the component dialogs quits the program.
		exit( 0 );
		return TRUE;
		break;
		
    case WM_COMMAND:

		switch ( LOWORD( wParam ) ) {
		case IDCANCEL:
//		case IDC_INTERRUPT:
			// Closing any of the component dialogs quits the program.
			EndDialog(hDlg, LOWORD(wParam));
			exit( 0 );
			return TRUE;
			break;
		}

		switch ( HIWORD( wParam ) ) {
		case BN_CLICKED:
		
			if ( IsDlgButtonChecked( hDlg, IDC_MASS1M ) ) onM = 1;
			else if ( IsDlgButtonChecked( hDlg, IDC_MASS2M ) ) onM = 2;
			else if ( IsDlgButtonChecked( hDlg, IDC_MASS3M ) ) onM = 3;
			else onM = 0;
						
			switch ( LOWORD( wParam ) ) {
			case IDC_CRADLEA:

				if ( IsDlgButtonChecked( hDlg, IDC_MASS1A ) ) inA = 1;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS2A ) ) inA = 2;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS3M ) ) inA = 3;
				else inA = 0;
				if ( !onM ) {
					if ( inA == 1 ) CheckDlgButton( hDlg, IDC_MASS1M, 1 ), CheckDlgButton( hDlg, IDC_MASS1A, 0 ); 
					if ( inA == 2 ) CheckDlgButton( hDlg, IDC_MASS2M, 1 ), CheckDlgButton( hDlg, IDC_MASS2A, 0 ); 
					if ( inA == 3 ) CheckDlgButton( hDlg, IDC_MASS3M, 1 ), CheckDlgButton( hDlg, IDC_MASS3A, 0 );
				}
				else if ( inA == 0 ) {
					if ( onM == 1 ) CheckDlgButton( hDlg, IDC_MASS1A, 1 ), CheckDlgButton( hDlg, IDC_MASS1M, 0 ); 
					if ( onM == 2 ) CheckDlgButton( hDlg, IDC_MASS2A, 1 ), CheckDlgButton( hDlg, IDC_MASS2M, 0 ); 
					if ( onM == 3 ) CheckDlgButton( hDlg, IDC_MASS3A, 1 ), CheckDlgButton( hDlg, IDC_MASS3M, 0 );
				}
				return TRUE;
				break;

			case IDC_CRADLEB:

				if ( IsDlgButtonChecked( hDlg, IDC_MASS1B ) ) inB = 1;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS2B ) ) inB = 2;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS3B ) ) inB = 3;
				else inB = 0;
				if ( !onM ) {
					if ( inB == 1 ) CheckDlgButton( hDlg, IDC_MASS1M, 1 ), CheckDlgButton( hDlg, IDC_MASS1B, 0 ); 
					if ( inB == 2 ) CheckDlgButton( hDlg, IDC_MASS2M, 1 ), CheckDlgButton( hDlg, IDC_MASS2B, 0 ); 
					if ( inB == 3 ) CheckDlgButton( hDlg, IDC_MASS3M, 1 ), CheckDlgButton( hDlg, IDC_MASS3B, 0 );
				}
				else if ( inB == 0 ) {
					if ( onM == 1 ) CheckDlgButton( hDlg, IDC_MASS1B, 1 ), CheckDlgButton( hDlg, IDC_MASS1M, 0 ); 
					if ( onM == 2 ) CheckDlgButton( hDlg, IDC_MASS2B, 1 ), CheckDlgButton( hDlg, IDC_MASS2M, 0 ); 
					if ( onM == 3 ) CheckDlgButton( hDlg, IDC_MASS3B, 1 ), CheckDlgButton( hDlg, IDC_MASS3M, 0 );
				}
				return TRUE;
				break;

			case IDC_CRADLEC:

				if ( IsDlgButtonChecked( hDlg, IDC_MASS1C ) ) inC = 1;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS2C ) ) inC = 2;
				else if ( IsDlgButtonChecked( hDlg, IDC_MASS3C ) ) inC = 3;
				else inC = 0;
				if ( !onM ) {
					if ( inC == 1 ) CheckDlgButton( hDlg, IDC_MASS1M, 1 ), CheckDlgButton( hDlg, IDC_MASS1C, 0 ); 
					if ( inC == 2 ) CheckDlgButton( hDlg, IDC_MASS2M, 1 ), CheckDlgButton( hDlg, IDC_MASS2C, 0 ); 
					if ( inC == 3 ) CheckDlgButton( hDlg, IDC_MASS3M, 1 ), CheckDlgButton( hDlg, IDC_MASS3C, 0 );
				}
				else if ( inC == 0 ) {
					if ( onM == 1 ) CheckDlgButton( hDlg, IDC_MASS1C, 1 ), CheckDlgButton( hDlg, IDC_MASS1M, 0 ); 
					if ( onM == 2 ) CheckDlgButton( hDlg, IDC_MASS2C, 1 ), CheckDlgButton( hDlg, IDC_MASS2M, 0 ); 
					if ( onM == 3 ) CheckDlgButton( hDlg, IDC_MASS3C, 1 ), CheckDlgButton( hDlg, IDC_MASS3M, 0 );
				}
				return TRUE;
				break;
			
			}

			return TRUE;
			break;
		}
		
	}
    return FALSE;
}


/**************************************************************************************/

// Reproduce the functionality of MessageBox(), but with illustrations and remaining
//  inside the DexSimulatorGUI box.


// Store here temporarily the information that is to be displayed.
// It gets put into the dialog by WM_INITDIALOG.
 char _illustrated_message_picture_filename[256] = "";
 HBITMAP _illustrated_message_picture_bitmap = NULL;

static char _illustrated_message_text[256] = "";
static char _illustrated_message_label[256] = "";

// This callback is used for 'popups'. When they close, the APP keeps on going.
BOOL CALLBACK _dexPopupCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemText( hDlg, IDC_MESSAGE, _illustrated_message_text );
		if ( _illustrated_message_picture_bitmap ) {
			SendDlgItemMessage( hDlg, IDC_PICTURE, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM) _illustrated_message_picture_bitmap );
		}

   		return TRUE;
		break;
		
	case WM_PAINT:
		// The following is a hack. DialogBox() disables the parent, but I want 
		//  the other controls to stay active. So I re-enable the parent.
		// Why is it in WM_PAINT? Because WM_INITDIALOG is too early.
		EnableWindow( GetParent( hDlg ), true );
   		return TRUE;
		break;

    case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		return TRUE;
		break;
		
    case WM_COMMAND:

		switch ( LOWORD( wParam ) ) {
		case IDCANCEL:
		case IDOK:
		case IDRETRY:
		case IDABORT:
		case IDIGNORE:
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
			break;
		}
	}
    return FALSE;
}


int IllustratedMessageBox( const char *picture, const char *message, const char *label, int buttons ) {

	int return_code;

	int code = GetLastError();

	// Store the information that is to be displayed temporarily, so that it can be put into the dialog by WM_INITDIALOG.
	// Be careful not to excede the limits of the buffers and be careful to have a null-terminated string.
	if ( picture ) {
		strncpy(  _illustrated_message_picture_filename, PictureFilenamePrefix, sizeof( _illustrated_message_picture_filename ) );
		strncpy( _illustrated_message_picture_filename + strlen( PictureFilenamePrefix ), 
			picture, 
			sizeof( _illustrated_message_picture_filename ) - strlen( PictureFilenamePrefix ) );
		_illustrated_message_picture_filename[sizeof( _illustrated_message_picture_filename ) - 1] = 0;
		_illustrated_message_picture_bitmap = (HBITMAP) LoadImage( NULL, _illustrated_message_picture_filename, IMAGE_BITMAP, 595 * .6, 421 * .6, LR_CREATEDIBSECTION | LR_LOADFROMFILE | LR_VGACOLOR );
	}
	else {
		_illustrated_message_picture_filename[0] = 0;
		_illustrated_message_picture_bitmap = NULL;
	}

	if ( message ) {
		strncpy( _illustrated_message_text, message, sizeof( _illustrated_message_text ) );
		_illustrated_message_text[sizeof( _illustrated_message_text ) - 1] = 0;
		// Strings coming from scripts have end of lines marked with "\n". Need to convert to a real newline.
		for ( char *ptr = _illustrated_message_text; *ptr; *ptr++ ) {
			if ( *ptr == '\\' && *(ptr+1) == 'n' ) {
				*ptr = '\r';
				*(ptr+1) = '\n';
			}
		}
	}
	else _illustrated_message_text[0] = 0;

	if ( label ) {
		strncpy( _illustrated_message_label, label, sizeof( _illustrated_message_label ) );
		_illustrated_message_label[sizeof( _illustrated_message_label ) - 1] = 0;
	}
	else _illustrated_message_label[0] = 0;

	if ( ( buttons & 0x0000000f ) == MB_ABORTRETRYIGNORE ) {
		return_code = DialogBox( app_instance, (LPCSTR) IDD_ABORTRETRYIGNORE, workspace_dlg, _dexPopupCallback );
	}
	else if ( ( buttons & 0x0000000f ) == MB_OKCANCEL ) {
		return_code = DialogBox( app_instance, (LPCSTR) IDD_OKCANCEL, workspace_dlg, _dexPopupCallback );
	}
	else {
		return_code = MessageBox( NULL, message, label, buttons );
	}

	return( return_code );

}

int fIllustratedMessageBox( int mb_type, const char *picture, const char *caption, const char *format, ... ) {
	
	int items;
	
	va_list args;
	char message[10240];
	
	va_start(args, format);
	items = vsprintf(message, format, args);
	va_end(args);
	
	return( IllustratedMessageBox( picture, message, caption, mb_type ) );
		
}

/**************************************************************************************/

void DexAddToLogGUI( const char *message ) {

	int i;

#if 0
	char date_str [9];
	char time_str [9];
	_strdate( date_str);
	_strtime( time_str );

	for ( i = 0; i < strlen( date_str ) && _dexLogNext < sizeof( _dexLog ) - 1; i++, _dexLogNext++ ) _dexLog[_dexLogNext] = date_str[i];
	if ( _dexLogNext < sizeof( _dexLog ) - 1 ) _dexLog[_dexLogNext++] = ' ';
	for ( i = 0; i < strlen( time_str ) && _dexLogNext < sizeof( _dexLog ) - 1; i++, _dexLogNext++ ) _dexLog[_dexLogNext] = time_str[i];
	if ( _dexLogNext < sizeof( _dexLog ) - 1 ) _dexLog[_dexLogNext++] = ' ';
#endif

	for ( i = 0; i < strlen( message ) && _dexLogNext < sizeof( _dexLog ) - 1; i++ ) {
		if ( message[i] == '\n' ) _dexLog[ _dexLogNext ] = '|';
		else _dexLog[ _dexLogNext ] = message[i];
		if ( _dexLogNext < sizeof( _dexLog ) - 1 ) _dexLogNext++;
	}
	if ( _dexLogNext < sizeof( _dexLog ) - 1 ) _dexLog[_dexLogNext++] = '\r';
	if ( _dexLogNext < sizeof( _dexLog ) - 1 ) _dexLog[_dexLogNext++] = '\n';
	_dexLog[ _dexLogNext ] = 0;

	if ( DexTimerTimeout( _refresh_timer ) ) {
		int lines;
		SendDlgItemMessage( workspace_dlg, IDC_LOG, WM_SETTEXT, NULL, (LPARAM) _dexLog );
		lines = SendDlgItemMessage( workspace_dlg, IDC_LOG, EM_GETLINECOUNT, (WPARAM) 0, (LPARAM) 0 );
		SendDlgItemMessage( workspace_dlg, IDC_LOG, EM_LINESCROLL, (WPARAM) 0, (LPARAM) lines );
		DexTimerSet( _refresh_timer, _refresh_rate );
	}
}

HWND DexInitGUI( HINSTANCE hInstance ) {

	HFONT hFont;

	// Share the instance handle with other routines.
	app_instance = hInstance;
	workspace_dlg = CreateDialog(hInstance, (LPCSTR)IDD_BACKGROUND, HWND_DESKTOP, dexDlgCallback );
	hFont = CreateFont (12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
	SendDlgItemMessage( workspace_dlg, IDC_LOG, WM_SETFONT, WPARAM (hFont), TRUE);
	_dexLog[ sizeof( _dexLog ) - 1 ] = 0;
	DexTimerSet( _refresh_timer, _refresh_rate );
	
	return( workspace_dlg );

}


HWND DexCreateMouseTrackerGUI( void ) {

	mouse_tracker_dlg = CreateDialog( app_instance, (LPCSTR)IDD_CONFIG, workspace_dlg, dexDlgCallback );
	CheckRadioButton( mouse_tracker_dlg, IDC_SEATED, IDC_SUPINE, IDC_SEATED ); 
	CheckRadioButton( mouse_tracker_dlg, IDC_LEFT, IDC_RIGHT, IDC_LEFT ); 
	CheckRadioButton( mouse_tracker_dlg, IDC_HORIZ, IDC_VERT, IDC_VERT );
	CheckRadioButton( mouse_tracker_dlg, IDC_FOLDED, IDC_EXTENDED, IDC_FOLDED );
	CheckDlgButton( mouse_tracker_dlg, IDC_CODA_POSITIONED, true );

	SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_INSERTSTRING, TRACKER_UNALIGNED,       (LPARAM) "  <not aligned>  " );
	SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_INSERTSTRING, TRACKER_ALIGNED_UPRIGHT, (LPARAM) "Aligned Seated" );
	SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_INSERTSTRING, TRACKER_ALIGNED_SUPINE, (LPARAM) "Aligned Supine" );
	SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_SETCURSEL, 0, NULL );

	ShowWindow( mouse_tracker_dlg, SW_SHOW );

	return( mouse_tracker_dlg );

}

HWND DexCreatePhotoCameraGUI( void ) {
	camera_dlg = CreateDialog( app_instance, (LPCSTR)IDD_CAMERA, HWND_DESKTOP, dexDlgCallback );
	ShowWindow( camera_dlg, SW_HIDE );
	return( camera_dlg );
}


HWND DexCreateMassGUI( void ) {

	// Create a dialog box to emulate selection of the extra masses.
	mass_dlg = CreateDialog( app_instance, (LPCSTR)IDD_MASS, workspace_dlg, dexDlgCallback );
	CheckRadioButton( mass_dlg, IDC_MASS1A, IDC_MASS1M, IDC_MASS1A ); 
	CheckRadioButton( mass_dlg, IDC_MASS2A, IDC_MASS2M, IDC_MASS2B ); 
	CheckRadioButton( mass_dlg, IDC_MASS3A, IDC_MASS3M, IDC_MASS3C ); 

	return( mass_dlg );

}


/**************************************************************************************/

// Save and restore the simulated configuration from one run to the next.

void SaveGUIState( void ) {

	FILE *fp;

	fp = fopen( "DexSimulatorApp.cfg", "w" );
	if ( !fp ) {
		MessageBox( NULL, "saveInitialState() failed.", "DexSimulatorApp", IDOK );
	}
	else {
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_SUPINE ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_SEATED ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_BOX_OCCLUDED ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_RIGHT ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_LEFT ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_BAR_OCCLUDED ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_HORIZ ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_VERT ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_FOLDED ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_EXTENDED ) ); 

		fprintf( fp, "%d ", SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_GETCURSEL, NULL, NULL ) );
		
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_CODA_POSITIONED ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_CODA_NOISY ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mouse_tracker_dlg, IDC_CODA_WOBBLY ) ); 

		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS1A ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS1B ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS1C ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS1M ) ); 

		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS2A ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS2B ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS2C ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS2M ) ); 

		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS3A ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS3B ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS3C ) ); 
		fprintf( fp, "%d ", IsDlgButtonChecked( mass_dlg, IDC_MASS3M ) ); 
		fclose( fp );
	}


}

void LoadGUIState( void ) {

	FILE	*fp;
	int		value;
	int		items;

	fp = fopen( "DexSimulatorApp.cfg", "r" );
	if ( !fp ) MessageBox( NULL, "loadInitialState() failed.", "DexSimulatorApp", IDOK );
	else {
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_SUPINE, value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_SUPINE, false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_SEATED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_SEATED , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_BOX_OCCLUDED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_BOX_OCCLUDED , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_RIGHT , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_RIGHT , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_LEFT , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_LEFT , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_BAR_OCCLUDED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_BAR_OCCLUDED , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_HORIZ , value );
		else  CheckDlgButton( mouse_tracker_dlg, IDC_HORIZ , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_VERT , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_VERT , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_FOLDED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_FOLDED , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_EXTENDED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_EXTENDED , false );

		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_SETCURSEL, value, NULL );
		else SendDlgItemMessage( mouse_tracker_dlg, IDC_ALIGNMENT, CB_SETCURSEL, 2, NULL );

		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_CODA_POSITIONED , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_CODA_POSITIONED , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_CODA_NOISY , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_CODA_NOISY , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mouse_tracker_dlg, IDC_CODA_WOBBLY , value );
		else CheckDlgButton( mouse_tracker_dlg, IDC_CODA_WOBBLY , value );

		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS1A , value );
		else CheckDlgButton( mass_dlg, IDC_MASS1A , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS1B , value );
		else CheckDlgButton( mass_dlg, IDC_MASS1B , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS1C , value );
		else CheckDlgButton( mass_dlg, IDC_MASS1C , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS1M , value );
		else CheckDlgButton( mass_dlg, IDC_MASS1M , false );

		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS2A , value );
		else CheckDlgButton( mass_dlg, IDC_MASS2A , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS2B , value );
		else CheckDlgButton( mass_dlg, IDC_MASS2B , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS2C , value );
		else CheckDlgButton( mass_dlg, IDC_MASS2C , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS2M , value );
		else CheckDlgButton( mass_dlg, IDC_MASS2M , false );

		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS3A , value );
		else CheckDlgButton( mass_dlg, IDC_MASS3A , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS3B , value );
		else CheckDlgButton( mass_dlg, IDC_MASS3B , false );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS3C , value );
		else CheckDlgButton( mass_dlg, IDC_MASS3C , true );
		items = fscanf( fp, "%d", &value );
		if ( items == 1 ) CheckDlgButton( mass_dlg, IDC_MASS3M , value );
		else CheckDlgButton( mass_dlg, IDC_MASS3M , false );

		fclose( fp );
	}

	SaveGUIState();
}
