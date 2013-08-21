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

#include <3dMatrix.h>
#include <OglDisplayInterface.h>
#include <Views.h>
#include <Layouts.h>

/*********************************************************************************/

//#define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/

// Common parameters.
double maxTrialDuration = 60.0;				// Trials can be up to this long, but may be shorter.

double baselineTime = 1.0;					// Duration of pause at first target before starting movement.
double initialMovementTime = 5.0;			// Time allowed to reach the starting position.
double continuousDropoutTimeLimit = 0.050;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double cumulativeDropoutTimeLimit = 1.000;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double beepDuration = BEEP_DURATION;
double flashDuration = 0.1;
double copTolerance = 10.0;					// Tolerance on how well the fingers are centered on the manipulandum.
double copForceThreshold = 0.25;			// Threshold of grip force to test if the manipulandum is in the hand.
double copWaitTime = 1.0;					// Gives time to achieve the centered grip. 
											// If it is short (eg 1s) it acts like a test of whether a centered grip is already achieved.


/*********************************************************************************/

// 2D Graphics
Display display; 
Layout  layout;
View	view, yz_view, cop_view;

int plot_screen_left = 225;
int plot_screen_top = 120;
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

HWND	dlg;
HWND	status_dlg;

double Vt[DEX_MAX_MARKER_FRAMES];

char *axis_name[] = { "X", "Y", "Z", "M" };

/*********************************************************************************/

// These are some helper functions that can be used in the task and procedures.

void BlinkAll ( DexApparatus *apparatus ) {

	apparatus->SetTargetState( ~0 );
	apparatus->Wait( flashDuration );
	apparatus->TargetsOff();
	apparatus->Wait( flashDuration );

}

void ShowStatus ( const char *message ) {
	ShowWindow( status_dlg, SW_SHOW );
	SetDlgItemText( status_dlg, IDC_STATUS_TEXT, message );
}

void HideStatus ( void ) {
	ShowWindow( status_dlg, SW_HIDE );
}

int ParseForPosture ( const char *cmd ) {
	if ( cmd && strstr( cmd, "-sup" ) ) return( SUPINE );
	else if ( strstr( cmd, "-up" ) ) return( UPRIGHT );
	else return( DEFAULT );
}


int ParseForEyeState ( const char *cmd ) {
	if ( cmd && strstr( cmd, "-open" ) ) return( OPEN );
	else if ( strstr( cmd, "-close" ) ) return( CLOSED );
	else return( DEFAULT );
}

int ParseForDirection ( DexApparatus *apparatus, const char *cmd, int &posture, int &bar_position, Vector3 &direction_vector, Quaternion &desired_orientation ) {

	int direction = VERTICAL;
	int pos = ParseForPosture( cmd );	

	if ( cmd && strstr( cmd, "-ver" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-hor" ) ) direction = HORIZONTAL;

	if ( pos == UPRIGHT ) {
		apparatus->CopyQuaternion( desired_orientation, uprightNullOrientation );
		if ( direction == HORIZONTAL ) {
			bar_position = TargetBarLeft;
			apparatus->CopyVector( direction_vector, apparatus->kVector );
		}
		if ( direction == VERTICAL ) {
			bar_position = TargetBarRight;
			apparatus->CopyVector( direction_vector, apparatus->jVector );
		}
	}
	else if ( pos == SUPINE ) {
		apparatus->CopyQuaternion( desired_orientation, supineNullOrientation );
		if ( direction == HORIZONTAL ) {
			bar_position = TargetBarLeft;
			apparatus->CopyVector( direction_vector, apparatus->jVector );
		}
		if ( direction == VERTICAL ) {
			bar_position = TargetBarRight;
			apparatus->CopyVector( direction_vector, apparatus->kVector );
		}
	}
	return( direction );
}

/**************************************************************************************/

void init_plots ( void ) {

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

void plot_data( DexApparatus *apparatus ) {

	int i, j, cnt;
	Vector3 delta;
	double max_time;
	double filtered, filter_constant = 1.0;

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
		ViewLine( view, 0.0,   copTolerance / 1000.0, samples,   copTolerance / 1000.0 );
		ViewLine( view, 0.0, - copTolerance / 1000.0, samples, - copTolerance / 1000.0 );
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

// Mesage handler for dialog box.

BOOL CALLBACK dexDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
        
		//      SetTimer( hDlg, 1, period * 1000, NULL );
		return TRUE;
		break;
		
    case WM_TIMER:
		return TRUE;
		break;
		
    case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		exit( 0 );
		return TRUE;
		break;
		
    case WM_COMMAND:
		switch ( LOWORD( wParam ) ) {
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			exit( 0 );
			return TRUE;
			break;
		}
		
	}
    return FALSE;
}



/**************************************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	
	DexApparatus *apparatus;
	DexTimer session_timer;
	
	TrackerType	tracker_type;
	TargetType	target_type;
	SoundType	sound_type;
	AdcType		adc_type;

	DexTracker	*tracker;
	DexTargets	*targets;
	DexSounds	*sounds;
	DexADC		*adc;	
	
	int task = TARGETED_TASK;
	char *script = "DexSampleScript.dex";
	int direction = VERTICAL;
	bool eyes_closed = false;
	bool use_compiler = false;

	int return_code;
	
	// Parse command line.
	
	// First, define the devices.
	if ( strstr( lpCmdLine, "-coda"   ) ) tracker_type = CODA_TRACKER;
	else if ( strstr( lpCmdLine, "-rt"     ) ) tracker_type = RTNET_TRACKER;
	else tracker_type = MOUSE_TRACKER;

	if ( strstr( lpCmdLine, "-glm"   ) ) {
		adc_type = GLM_ADC;
		target_type = GLM_TARGETS;
	}
	else {
		adc_type = MOUSE_ADC;
		target_type = SCREEN_TARGETS;
	}

	if ( strstr( lpCmdLine, "-blaster"   ) ) sound_type = SOUNDBLASTER_SOUNDS;
	else sound_type = SCREEN_SOUNDS;

	// Now specify what task or protocol to run.

	if ( strstr( lpCmdLine, "-osc"    ) ) task = OSCILLATION_TASK;
	if ( strstr( lpCmdLine, "-oscillations"    ) ) task = OSCILLATION_TASK;
	if ( strstr( lpCmdLine, "-targeted"    ) ) task = TARGETED_TASK;
	if ( strstr( lpCmdLine, "-discrete"    ) ) task = DISCRETE_TASK;
	if ( strstr( lpCmdLine, "-coll"   ) ) task = COLLISION_TASK;
	if ( strstr( lpCmdLine, "-collisions"   ) ) task = COLLISION_TASK;
	if ( strstr( lpCmdLine, "-friction"   ) ) task = FRICTION_TASK;

	if ( strstr( lpCmdLine, "-script" ) ) task = RUN_SCRIPT;
	if ( strstr( lpCmdLine, "-calib"  ) ) task = CALIBRATE_TARGETS;
	if ( strstr( lpCmdLine, "-install"  ) ) task = INSTALL_PROCEDURE;

	// We can set some parameters for certain tasks.

	if ( strstr( lpCmdLine, "-horiz"  ) ) direction = HORIZONTAL;
	if ( strstr( lpCmdLine, "-vert"  ) ) direction = VERTICAL;

	if ( strstr( lpCmdLine, "-open"  ) ) eyes_closed = false;
	if ( strstr( lpCmdLine, "-closed"  ) ) eyes_closed = true;

	if ( strstr( lpCmdLine, "-compile"  ) ) use_compiler = true;


	switch ( tracker_type ) {

	case MOUSE_TRACKER:

		dlg = CreateDialog(hInstance, (LPCSTR)IDD_CONFIG, HWND_DESKTOP, dexDlgCallback );
		CheckRadioButton( dlg, IDC_SEATED, IDC_SUPINE, IDC_SEATED ); 
		CheckRadioButton( dlg, IDC_LEFT, IDC_RIGHT, IDC_LEFT ); 
		CheckRadioButton( dlg, IDC_HORIZ, IDC_VERT, IDC_VERT );
		CheckRadioButton( dlg, IDC_FOLDED, IDC_EXTENDED, IDC_FOLDED );
		CheckDlgButton( dlg, IDC_CODA_ALIGNED, true );
		CheckDlgButton( dlg, IDC_CODA_POSITIONED, true );
		ShowWindow( dlg, SW_SHOW );

		tracker = new DexMouseTracker( dlg );
		break;

	case CODA_TRACKER:
		tracker = new DexCodaTracker();
		break;

	case RTNET_TRACKER:
		tracker = new DexRTnetTracker();
		break;

	default:
		MessageBox( NULL, "Unkown tracker type.", "Error", MB_OK );

	}
		
	switch ( target_type ) {

	case GLM_TARGETS:
		targets = new DexNiDaqTargets(); // GLM
		break;

	case SCREEN_TARGETS:
		targets = new DexScreenTargets(); 
		break;

	default:
		MessageBox( NULL, "Unknown target type.", "Error", MB_OK );

	}

	switch ( sound_type ) {

	case SOUNDBLASTER_SOUNDS:
		sounds = new DexScreenSounds(); // Soundblaster sounds not yet implemented.
		break;

	case SCREEN_SOUNDS:
		sounds = new DexScreenSounds(); 
		break;

	default:
		MessageBox( NULL, "Unknown sound type.", "Error", MB_OK );

	}
	
	switch ( adc_type ) {

	case GLM_ADC:
		adc = new DexNiDaqADC(); // GLM targets not yet implemented.
		break;

	case MOUSE_ADC:
		adc = new DexMouseADC( dlg, GLM_CHANNELS ); 
		break;

	default:
		MessageBox( NULL, "Unknown adc type.", "Error", MB_OK );

	}

	// Create an apparatus by piecing together the components.
	if ( use_compiler ) apparatus = new DexCompiler( tracker, targets, sounds, adc, "TestScript.dex" );
	else apparatus = new DexApparatus( tracker, targets, sounds, adc );

	apparatus->Initialize();

	// Keep track of the elapsed time from the start of the first trial.
	DexTimerStart( session_timer );

	init_plots();

	// Create a dialog box locally that we can display while the data is being saved.
	status_dlg = CreateDialog(hInstance, (LPCSTR)IDD_STATUS, HWND_DESKTOP, dexDlgCallback );
	HideStatus();
	
	// Run one of the protocols.
	
	switch ( task ) {

	case CALIBRATE_TARGETS:
		while ( RETRY_EXIT == ( return_code = RunTargetCalibration( apparatus, lpCmdLine ) ) );
		break;
		
	case RUN_SCRIPT:
		while ( RETRY_EXIT == ( return_code = RunScript( apparatus, script ) ) );
		if ( return_code != ABORT_EXIT ) plot_data( apparatus );
		break;
		
	case OSCILLATION_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		while ( RETRY_EXIT == ( return_code = RunOscillations( apparatus, lpCmdLine ) ) );
		if ( return_code != ABORT_EXIT ) plot_data( apparatus );
		break;
		
	case COLLISION_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		while ( RETRY_EXIT == ( return_code = RunCollisions( apparatus, lpCmdLine ) ) );
		if ( return_code != ABORT_EXIT ) plot_data( apparatus );
		break;

	case FRICTION_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		while ( RETRY_EXIT == ( return_code = RunFrictionMeasurement( apparatus, lpCmdLine ) ) );
		if ( return_code != ABORT_EXIT ) plot_data( apparatus );
		break;

	case TARGETED_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		while ( RETRY_EXIT == ( return_code = RunTargeted( apparatus, lpCmdLine ) ) );
		if ( return_code != ABORT_EXIT && !use_compiler ) plot_data( apparatus );
		break;

	case DISCRETE_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		while ( RETRY_EXIT == ( return_code = RunDiscrete( apparatus, lpCmdLine ) ) );
		if ( return_code != ABORT_EXIT ) plot_data( apparatus );
		break;

	case INSTALL_PROCEDURE:
		while ( RETRY_EXIT == RunInstall( apparatus, lpCmdLine ) );
		break;

	}
	
	apparatus->Quit();
	return 0;
}



