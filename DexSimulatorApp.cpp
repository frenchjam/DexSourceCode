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

#include <fOutputDebugString.h>
#include <DateTimeString.h>

/*********************************************************************************/

// Parameters that are common to more than one task.

double maxTrialDuration = 60.0;				// Trials can be up to this long, but may be shorter.

double baselineTime = 1.0;					// Duration of pause at first target before starting movement.
double initialMovementTime = 5.0;			// Time allowed to reach the starting position.
double continuousDropoutTimeLimit = 0.050;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double cumulativeDropoutTimeLimit = 1.000;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double beepDuration = BEEP_DURATION;
double flashDuration = 0.2;
double copTolerance = 10.0;					// Tolerance on how well the fingers are centered on the manipulandum.
double copForceThreshold = 0.25;			// Threshold of grip force to test if the manipulandum is in the hand.
double copWaitTime = 1.0;					// Gives time to achieve the centered grip. 
											// If it is short (eg 1s) it acts like a test of whether a centered grip is already achieved.

char inputScript[256] = "DexSampleScript.dex";
char outputScript[256] = "DexSampleScript.dex";

char *OkToContinue ="";

int	next_directive = 0;						// Counter for showing a sequence of instructions to the subject.

/*********************************************************************************/

// These are some helper functions that can be used in the task and procedures.
// The first ones provide routines to present a set of task instructions in a stereotypical fashion.
// By making these helper functions, it is easier to maintain a common format.

void RestartDirectives( DexApparatus *apparatus ) {
	next_directive = 0;
}

void GiveDirective( DexApparatus *apparatus, const char *directive, const char *picture ) {
	next_directive++;
	int status = apparatus->fWaitSubjectReady( picture, "                           TASK REMINDER   (%d)\n%s%s", next_directive, directive, OkToContinue );
	if ( status == ABORT_EXIT ) exit( status );
}

void ReadyToGo( DexApparatus *apparatus ) {
	int status = apparatus->fWaitSubjectReady( NULL, "READY?\n\nPress <OK> to start ..." );
	if ( status == ABORT_EXIT ) exit( status );
}

/*********************************************************************************/

void BlinkAll ( DexApparatus *apparatus ) {

	apparatus->SetTargetState( ~0 );
	apparatus->Wait( flashDuration );
	apparatus->TargetsOff();
	apparatus->Wait( flashDuration );

}

/*********************************************************************************/

// Parse command line commands.

DexMass ParseForMass ( const char *cmd ) {
	if ( !cmd ) return( MassIndifferent );
	if ( strstr( cmd, "-400" ) ) return( MassSmall );
	else if ( strstr( cmd, "-600" ) ) return( MassMedium );
	else if ( strstr( cmd, "-800" ) ) return( MassLarge );
	else if ( strstr( cmd, "-nomass" ) ) return( MassNone );
	else return( MassSmall );
}


DexSubjectPosture ParseForPosture ( const char *cmd ) {
	if ( !cmd ) return( PostureIndifferent );
	if ( strstr( cmd, "-sup" )) return( PostureSupine );
	else if ( strstr( cmd, "-up" )) return( PostureSeated );
	else return( PostureSeated );
}


int ParseForEyeState ( const char *cmd ) {
	if ( !cmd ) return( OPEN );
	if ( strstr( cmd, "-open" ) ) return( OPEN );
	else if ( strstr( cmd, "-close" ) ) return( CLOSED );
	else return( OPEN );
}

bool ParseForPrep ( const char *cmd ) {
	if ( !cmd ) return( false );
	if ( strstr( cmd, "-prep" ) ) return( true );
	else return( false );
}

char *ParseForTargetFile ( const char *cmd ) {
	static char filename[1024] = "";
	char *ptr;

	if ( !cmd ) return( NULL );
	else if ( ptr = strstr( cmd, "-targets=" ) ) {
		sscanf( ptr + strlen( "-targets=" ), "%s", filename);
		return( filename );
	}
	else return( NULL );
}

char *ParseForDelayFile ( const char *cmd ) {
	static char filename[1024] = "";
	char *ptr;

	if ( !cmd ) return( NULL );
	else if ( ptr = strstr( cmd, "-delays=" ) ) sscanf( ptr + strlen( "-delays=" ), "%s", filename);
	else ptr = NULL;
	return( filename );
}

// Load a sequence of integers, e.g. a list of target IDs.
int LoadSequence( const char *filename, int *sequence, const int max_entries ) {

	FILE *fp;
	char path[1024];
	int	 count = 0;

	strcpy( path, "..\\DexSequences\\" );
	strcat( path, filename );
	if ( !( fp = fopen( path, "r" ) ) ) {
		fIllustratedMessageBox( MB_OK, "alert.bmp", "DexSimulatorApp", "Error opening file %s for read.", path );
		exit( -1 );
	}

	while ( 1 == fscanf( fp, "%d", &sequence[count] ) ) count++;
	
	fclose( fp );

	return( count );

}

// Load a sequence of floating point numbers, e.g. time delays in seconds.
int LoadSequence( const char *filename, float *sequence, const int max_entries ) {

	FILE *fp;
	char path[1024];
	int	 count = 0;

	strcpy( path, "..\\DexSequences\\" );
	strcat( path, filename );
	if ( !( fp = fopen( path, "r" ) ) ) {
		fIllustratedMessageBox( MB_OK, "alert.bmp", "DexSimulatorApp", "Error opening file %s for read.", path );
		exit( -1 );
	}

	while ( 1 == fscanf( fp, "%f", &sequence[count] ) ) count++;
	
	fclose( fp );

	return( count );

}

int ParseForDirection ( DexApparatus *apparatus, const char *cmd, DexSubjectPosture &posture, DexTargetBarConfiguration &bar_position, Vector3 &direction_vector, Quaternion &desired_orientation ) {

	int direction = VERTICAL;
	DexSubjectPosture pos = ParseForPosture( cmd );	

	if ( !cmd ) {
		posture = PostureSeated;
		bar_position = TargetBarRight;
		apparatus->CopyVector( direction_vector, apparatus->jVector );
		apparatus->CopyQuaternion( desired_orientation, uprightNullOrientation );
		return( direction );
	}

	if ( strstr( cmd, "-ver" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-hor" ) ) direction = HORIZONTAL;

	if ( pos == PostureSupine ) {
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
	else {
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
	return( direction );
}

/**************************************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	
	DexApparatus *apparatus;
	
	TrackerType	tracker_type;
	TargetType	target_type;
	SoundType	sound_type;
	AdcType		adc_type;

	DexTracker	*tracker;
	DexTargets	*targets;
	DexSounds	*sounds;
	DexADC		*adc;	
	DexMonitorServer *monitor;
	
	int task = TARGETED_TASK;
	bool compile = false;
	bool raz = false;

	int return_code;

	HWND work_dlg, mouse_dlg, mass_dlg;
	
	// Parse command line.
	
	// First, define the devices.

	// By default, the mouse simulates movement of the manipulandum.
	tracker_type = MOUSE_TRACKER;

	// By default, simulate sounds on the screen.
	sound_type = SCREEN_SOUNDS;
	// By default, simulate analog inputs with mouse movements.
	adc_type = MOUSE_ADC;
	// By default, use the screen to present targets.
	target_type = SCREEN_TARGETS;

	if ( strstr( lpCmdLine, "-coda"   ) ) tracker_type = CODA_TRACKER;
	if ( strstr( lpCmdLine, "-rt"     ) ) tracker_type = RTNET_TRACKER;

	if ( strstr( lpCmdLine, "-glm"   ) ) {
		adc_type = GLM_ADC;
		target_type = GLM_TARGETS;
		sound_type = GLM_SOUNDS;
	}

	if ( strstr( lpCmdLine, "-blaster"   ) ) sound_type = SOUNDBLASTER_SOUNDS;

	// Now specify what task or protocol to run.

	if ( strstr( lpCmdLine, "-osc"    ) ) task = OSCILLATION_TASK;
	if ( strstr( lpCmdLine, "-oscillations"    ) ) task = OSCILLATION_TASK;
	if ( strstr( lpCmdLine, "-targeted"    ) ) task = TARGETED_TASK;
	if ( strstr( lpCmdLine, "-discrete"    ) ) task = DISCRETE_TASK;
	if ( strstr( lpCmdLine, "-coll"   ) ) task = COLLISION_TASK;
	if ( strstr( lpCmdLine, "-collisions"   ) ) task = COLLISION_TASK;
	if ( strstr( lpCmdLine, "-friction"   ) ) task = FRICTION_TASK;
	if ( strstr( lpCmdLine, "-calib"  ) ) task = CALIBRATE_TARGETS;
	if ( strstr( lpCmdLine, "-install"  ) ) task = INSTALL_PROCEDURE;
	if ( strstr( lpCmdLine, "-offsets"  ) ) task = OFFSETS_TASK;

	// Resetting the offsets on the force sensors can be considered as a task in itself.
	// Here we give the opportunity to execute it in addition to the specified task, making
	//  it available to be run at the start of a simuation session.

	if ( strstr( lpCmdLine, "-raz"  ) ) raz = true;

	// This should invoke the command interpreter on the specified script.
	// For the moment, the interpreter is not working.
	if ( strstr( lpCmdLine, "-script" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-script=" ) ) ) {
			sscanf( ptr + strlen( "-script=" ), "%s", inputScript );
		}
		task = RUN_SCRIPT;	
	}

	// This says to compile the C++ code into a DEX script, instead of executing it.
	// You can specify the name of the output script.
	if ( strstr( lpCmdLine, "-compile" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-compile=" ) ) ) {
			sscanf( ptr + strlen( "-compile=" ), "%s", outputScript );
		}
		compile = true;
	}

	// Now we create the component devices. This need not be done if we are in compiler mode.

	if ( !compile ) {

		// Create a window for plotting the data.
		DexInitPlots();

		// Create a window to work in.
		work_dlg = DexInitGUI( hInstance );

		switch ( tracker_type ) {

		case MOUSE_TRACKER:

			mouse_dlg = DexCreateMouseTrackerGUI();
			tracker = new DexMouseTracker( mouse_dlg );
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

		case GLM_SOUNDS:
			sounds = new DexNiDaqAdcSounds(); 
			break;

		default:
			MessageBox( NULL, "Unknown sound type.", "Error", MB_OK );

		}
		
		switch ( adc_type ) {

		case GLM_ADC:
			adc = new DexNiDaqADC(); // GLM targets not yet implemented.
			break;

		case MOUSE_ADC:
			adc = new DexMouseADC( mouse_tracker_dlg, GLM_CHANNELS ); 
			break;

		default:
			MessageBox( NULL, "Unknown adc type.", "Error", MB_OK );

		}

		monitor = new DexMonitorServerGUI( targets->nVerticalTargets, targets->nHorizontalTargets, tracker->nCodas );


		// Create a dialog box to emulate selection of the extra masses.
		mass_dlg = DexCreateMassGUI();
		apparatus = new DexApparatus( tracker, targets, sounds, adc, monitor, work_dlg, mass_dlg );

	}

	else apparatus = new DexCompiler( outputScript );

	// Create the apparatus.
	apparatus->Initialize();

	// If we are running the task on the simulator, give the operator a chance to set the initial hardware configutation.
	if ( !compile ) {
		LoadGUIState();
		return_code = apparatus->WaitSubjectReady( "", "Use the GUI to set the initial configuration.\nPress OK when ready to continue." );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		SaveGUIState();
	}

	// If the command line flag was set to do the transducer offset cancellation, then do it.
	if ( raz ) {
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
	}

	// Run one of the protocols.
	switch ( task ) {

	case CALIBRATE_TARGETS:
		while ( RETRY_EXIT == ( return_code = RunTargetCalibration( apparatus, lpCmdLine ) ) );
		apparatus->Quit();
		return 0;
		break;
		
	case RUN_SCRIPT:
		while ( RETRY_EXIT == ( return_code = RunScript( apparatus, inputScript ) ) );
		break;
		
	case OFFSETS_TASK:
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		break;
		
	case OSCILLATION_TASK:
		while ( RETRY_EXIT == ( return_code = RunOscillations( apparatus, lpCmdLine ) ) );
		break;
		
	case COLLISION_TASK:
		while ( RETRY_EXIT == ( return_code = RunCollisions( apparatus, lpCmdLine ) ) );
		break;

	case FRICTION_TASK:
		while ( RETRY_EXIT == ( return_code = RunFrictionMeasurement( apparatus, lpCmdLine ) ) );
		break;

	case TARGETED_TASK:
		while ( RETRY_EXIT == ( return_code = RunTargeted( apparatus, lpCmdLine ) ) );
		break;

	case DISCRETE_TASK:
		while ( RETRY_EXIT == ( return_code = RunDiscrete( apparatus, lpCmdLine ) ) );
		break;

	case INSTALL_PROCEDURE:
		while ( RETRY_EXIT == ( return_code = RunInstall( apparatus, lpCmdLine ) ) );
		break;

	}
	if ( return_code != ABORT_EXIT && !compile ) DexPlotData( apparatus );
	
	SaveGUIState();
	apparatus->Quit();
	return 0;
}



