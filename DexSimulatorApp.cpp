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

double maxTrialDuration = 120.0;			// Used to set the maximum duration of an acquisition.
											// Trials can be up to this long, but may be shorter.
											// May not be applicable to the DEX hardware.

double baselineDuration = 1.0;				// Duration of pause at first target before starting movement.
double initialMovementTime = 5.0;			// Time allowed to reach the starting position.
double continuousDropoutTimeLimit = 0.050;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double cumulativeDropoutTimeLimit = 1.000;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double beepDuration = BEEP_DURATION;
double flashDuration = 0.2;
double copTolerance = 10.0;					// Tolerance on how well the fingers are centered on the manipulandum.
double copForceThreshold = 0.25;			// Threshold of grip force to test if the manipulandum is in the hand.
double copWaitTime = 10.0;					// Gives time to achieve the centered grip. 
											// If it is short (eg 1s) it acts like a test of whether a centered grip is already achieved.

char outputScript[256] = "DexSampleScript.dex";
char inputScript[256] = "DexSampleScript.dex";
char inputProtocol[256] = "DexSampleProtocol.dex";
char inputSession[256] = "DexSampleSession.dex";
char inputSubject[256] = "users.dex";

/*********************************************************************************/

// Some common messages.

char *MsgReadyToStart = "Place the manipulandum in any cradle and lock in place.\nRemove hand and press <OK> to start.";
char *MsgReadyToStartOpen = "Place the manipulandum in any cradle.\nRemove hand and press <OK> to start.\nKeep your eyes OPEN.";
char *MsgReadyToStartClosed = "Place the manipulandum in any cradle.\nRemove hand and press <OK> to start.\nKeep your eyes CLOSED.";
char *MsgGripNotCentered = "Manipulandum not in hand \n      Or      \n Fingers not centered.";
char *MsgTooLongToReachTarget = "Too long to reach desired target.";
char *MsgCheckGripCentered = "Pick up the manipulandum with the mass. Adjust until the grip is centered.";
char *MsgMoveToBlinkingTarget = "Trial Started. Move to blinking target.";
char *MsgTrialOver = "Trial terminated.\nPlace the maniplandum in the empty cradle.";
char *MsgAcquiringBaseline = "Acquiring baseline. Please wait ...";
char *MsgQueryReadySeated = "Preparing to start the task.\nVerify that you are seated with your seat belts fastened.%s";
char *MsgQueryReadySupine = "Preparing to start the task.\nVerify that you are lying down with your seat belts fastened.%s";
char *InstructPickUpManipulandum = "You will first pick up the manipulandum with thumb and index finger centered.";
char *OkToContinue ="";

/*********************************************************************************/

#define NUMBER_OF_PROGRESS_BARS	10
void AnalysisProgress( DexApparatus *apparatus, int which, int total, const char *message ) {

	char picture[1024];
	int bar = (NUMBER_OF_PROGRESS_BARS * which) / total;

	if ( bar >= NUMBER_OF_PROGRESS_BARS ) strcpy( picture, "PrgBarF.bmp" );
	else sprintf( picture, "PrgBar%1d.bmp", bar );
	apparatus->ShowStatus( message, picture );

}

/*********************************************************************************/

// These are some helper functions that can be used in the task and procedures.
// The first ones provide routines to present a set of task instructions in a stereotypical fashion.
// By making these helper functions, it is easier to maintain a common format.

int	n_directives = 0;						// Counter for showing a sequence of instructions to the subject.
char directive_text[128][128];
char directive_picture[128][32];

void RestartDirectives( DexApparatus *apparatus ) {
	n_directives = 0;
}

void AddDirective( DexApparatus *apparatus, const char *directive, const char *picture ) {
	strncpy( directive_text[n_directives], directive, 128 ); directive_text[n_directives][127] = 0;
	strcpy( directive_picture[n_directives], picture );
	n_directives++;
}

void ShowDirectives( DexApparatus *apparatus ) {

	for ( int i = 0; i < n_directives; i++ ) {
		int status = apparatus->fWaitSubjectReady( directive_picture[i], "TASK PREVIEW (%d/%d): (Don't start!)\n%s%s", i + 1, n_directives, directive_text[i], OkToContinue );
		if ( status == ABORT_EXIT ) exit( status );
	}
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

void SignalEndOfRecording ( DexApparatus *apparatus ) {

	for ( int i = 0; i < 1; i++ ) {
		apparatus->SetTargetState( ~0 );
		apparatus->Beep();
		apparatus->Wait( flashDuration );
		apparatus->TargetsOff();
		apparatus->Wait( flashDuration );
	}

}

/*********************************************************************************/

// Parse command line commands.

DexMass ParseForMass ( const char *cmd ) {
	if ( !cmd ) return( MassIndifferent );
	if ( strstr( cmd, "-4" ) ) return( MassSmall );
	else if ( strstr( cmd, "-6" ) ) return( MassMedium );
	else if ( strstr( cmd, "-8" ) ) return( MassLarge );
	else if ( strstr( cmd, "-nomass" ) ) return( MassNone );
	else return( MassSmall );
}


DexSubjectPosture ParseForPosture ( const char *cmd ) {
	if ( !cmd ) return( PostureIndifferent );
	if ( strstr( cmd, "-supine" )) return( PostureSupine );
	else if ( strstr( cmd, "-Supine" )) return( PostureSupine );
	else if ( strstr( cmd, "-upright" )) return( PostureSeated );
	else if ( strstr( cmd, "-Upright" )) return( PostureSeated );
	else if ( strstr( cmd, "-seated" )) return( PostureSeated );
	else if ( strstr( cmd, "-Seated" )) return( PostureSeated );
	else return( PostureSeated );
}


int ParseForEyeState ( const char *cmd ) {
	if ( !cmd ) return( OPEN );
	if ( strstr( cmd, "-open" ) ) return( OPEN );
	else if ( strstr( cmd, "-Open" ) ) return( OPEN );
	else if ( strstr( cmd, "-closed" ) ) return( CLOSED );
	else if ( strstr( cmd, "-Closed" ) ) return( CLOSED );
	else return( OPEN );
}

bool ParseForBool( const char *cmd, const char *flag ) {
	if ( !cmd ) return( false );
	if ( strstr( cmd, flag ) ) return( true );
	else return( false );
}


bool ParseForPrep ( const char *cmd ) {
	return( ParseForBool( cmd, "-prep" ) );
}

#define BUFFER_N	10
char *ParseForFilename ( const char *cmd, const char *key ) {

	static char filename[BUFFER_N][1024];
	static call = 0;
	char *ptr;

	call = (call++) % BUFFER_N;
	if ( !cmd ) return( NULL );
	else if ( ptr = strstr( cmd, key ) ) {
		sscanf( ptr + strlen( key ) + 1, "%s", filename[call] ); // The '+1' takes care of the = sign, if there is one, or a space.
		return( filename[call] );
	}
	else return( NULL );
}

char *ParseForTargetFile ( const char *cmd ) {
	return( ParseForFilename( cmd, "-targets" ) );
}

char *ParseForRangeFile ( const char *cmd ) {
	return( ParseForFilename( cmd, "-range" ) );
}

char *ParseForDelayFile ( const char *cmd ) {
	return( ParseForFilename( cmd, "-delays" ) );
}

/************************************************************************************************************************************/

// Load a sequence of integers, e.g. a list of target IDs.
// There may be more than one sequence in the file.
// There may also be a different set of sequences for different size subjects.
// The filename can contain a modifier that tells which of the sequences to use in the file. 

int LoadSequence(  int *sequence, const char *filename ) {

	FILE *fp;
	char path[1024];
	int	 count = 0;
	char name[1024], *specifier, *ptr;

	int index = 1;
	int sequences = 1;
	int sizes = 1;
	int subject_size = 0;
	char subject_size_string[64];

	int value[N_SIZES][N_SEQUENCES];
	int n = 0, c;

	// Parse the filename for the subject size and sequence number qualifiers.
	strcpy( name, filename );
	if ( ptr = strstr( name, ":" ) ) {
		*ptr = 0; // Terminate the name at the start of the specifier.
		specifier = ptr + 1;
		if ( 2 == sscanf( specifier, "%d%s", &index, subject_size_string ) ) {
			sequences = N_SEQUENCES;
			sizes = N_SIZES;
		}
		else if ( 1 == sscanf( specifier, "%d", &index ) ) {
			sizes = 1;
			sequences = N_SEQUENCES; 
			strcpy( subject_size_string, "S" );
		}
		else if ( 1 == sscanf( specifier, "%s", subject_size_string ) ) {
			sequences = 1;
			sizes = N_SIZES;
			index = 1;
		}
		else {
			sizes = 1;
			sequences = 1; 
			strcpy( subject_size_string, "S" );
			index = 1;
		}

		switch ( subject_size_string[0] ) {
		case 'L':
		case 'l':
			subject_size = LARGE;
			break;
		case 'M':
		case 'm':
			subject_size = MEDIUM;
			break;
		case 'S':
		case 's':
		default:
			subject_size = SMALL;
			break;
		}

	}
				
	strcpy( path, "..\\DexSequences\\" );
	strcat( path, name );
	if ( !( fp = fopen( path, "r" ) ) ) {
		fIllustratedMessageBox( MB_OK, "alert.bmp", "DexSimulatorApp", "Error opening file %s for read.", path );
		exit( -1 );
	}
	// If there is supposed to be more than one subject size, skip that header line.
	// I have to implement my own fgets() because of incompatibility between mac and PC text files.
	if ( sizes > 1 ) do c = fgetc( fp ); while ( c != '\n' && c != '\r' );
	// If there is supposed to be more than one sequence, skip that header line.
	if ( sequences > 1 ) do c = fgetc( fp ); while ( c != '\n' && c != '\r' );
	// If we ask for too many sequeces, wrap around.
	index = index % sequences;
	do {

		for ( int sz = 0; sz < sizes; sz++ ) {
			for ( int seq = 0; seq < sequences; seq++ ) {
				n = fscanf( fp, "%d", &value[sz][seq] );
			}
		}
		if ( n == 1 ) sequence[count++] = value[subject_size][index - 1];

	} while ( n == 1 );

	fclose( fp );
	return( count );

}

// Load in a triplet of targets, defined as LOWER, MIDDLE and UPPER.
// These are used in the discrete and oscillations protocols to define the amplitude of the movements.
// The file may have a single line with three values, or it may have 3 lines of 3 values, corresponding to 
//  a small, medium or large subject. A qualifier (:S, :M, :L) appended to the filename is used to select which.
void LoadTargetRange( int limits[3], const char *filename ) {

	FILE *fp;
	char path[1024];
	char name[1024], *specifier, *ptr;
	char line[1024];
	int repeat;

	strcpy( name, filename );
	if ( ptr = strstr( name, ":" ) ) {
		*ptr = 0; // Terminate the name at the start of the specifier.
		specifier = ptr + 1;
		if ( toupper( *specifier ) == 'L' ) repeat = 3;
		else if ( toupper( *specifier ) == 'M' ) repeat = 2;
		else repeat = 1;
	}
	else repeat = 1;

	strcpy( path, "..\\DexSequences\\" );
	strcat( path, name );
	if ( !( fp = fopen( path, "r" ) ) ) {
		fIllustratedMessageBox( MB_OK, "alert.bmp", "DexSimulatorApp", "Error opening file %s for read.", path );
		exit( -1 );
	}

	// First header line shows small, medium large.
	fgets( line, sizeof( line ), fp );
	// Second shows lower, middle, upper.
	fgets( line, sizeof( line ), fp );
	// Now come the triplets.
	for ( int r = 0; r < repeat; r++ ) {
		fscanf( fp, "%d %d %d", &limits[LOWER], &limits[MIDDLE], &limits[UPPER] );
	}
	fclose( fp );
}


int LoadSequence(  double *sequence, const char *filename ) {

	FILE *fp;
	char path[1024];
	int	 count = 0;
	char name[1024], *specifier, *ptr;

	int index = 1;
	int sequences = 1;
	int sizes = 1;
	int subject_size = 0;
	char subject_size_string[64];

	double value[N_SIZES][N_SEQUENCES];
	int n = 0, c;

	// Parse the filename for the subject size and sequence number qualifiers.
	strcpy( name, filename );
	if ( ptr = strstr( name, ":" ) ) {
		*ptr = 0; // Terminate the name at the start of the specifier.
		specifier = ptr + 1;
		if ( 2 == sscanf( specifier, "%d%s", &index, subject_size_string ) ) {
			sequences = N_SEQUENCES;
			sizes = N_SIZES;
		}
		else if ( 1 == sscanf( specifier, "%d", &index ) ) {
			sizes = 1;
			sequences = N_SEQUENCES; 
			strcpy( subject_size_string, "S" );
		}
		else if ( 1 == sscanf( specifier, "%s", subject_size_string ) ) {
			sequences = 1;
			sizes = N_SIZES;
			index = 1;
		}
		else {
			sizes = 1;
			sequences = 1; 
			strcpy( subject_size_string, "S" );
			index = 1;
		}

		switch ( subject_size_string[0] ) {
		case 'L':
		case 'l':
			subject_size = LARGE;
			break;
		case 'M':
		case 'm':
			subject_size = MEDIUM;
			break;
		case 'S':
		case 's':
		default:
			subject_size = SMALL;
			break;
		}

	}
				
	strcpy( path, "..\\DexSequences\\" );
	strcat( path, name );
	if ( !( fp = fopen( path, "r" ) ) ) {
		fIllustratedMessageBox( MB_OK, "alert.bmp", "DexSimulatorApp", "Error opening file %s for read.", path );
		exit( -1 );
	}
	
	// If there is supposed to be more than one subject size, skip that header line.
	if ( sizes > 1 ) do c = fgetc( fp ); while ( c != '\n' && c != '\r' );
	// If there is supposed to be more than one sequence, skip that header line.
	if ( sequences > 1 ) do c = fgetc( fp ); while ( c != '\n' && c != '\r' );
	// If we ask for too many sequeces, wrap around.
	index = index % sequences;

	do {

		for ( int sz = 0; sz < sizes; sz++ ) {
			for ( int seq = 0; seq < sequences; seq++ ) {
				n = fscanf( fp, "%lf", &value[sz][seq] );
			}
		}
		if ( n == 1 ) sequence[count++] = value[subject_size][index - 1];

	} while ( n == 1 );

	fclose( fp );
	return( count );

}

int ParseForDirection ( DexApparatus *apparatus, const char *cmd ) {

	int direction = VERTICAL;
	if ( strstr( cmd, "-vertical" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-Vertical" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-vert" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-ver" ) ) direction = VERTICAL;
	else if ( strstr( cmd, "-Horizontal" ) ) direction = HORIZONTAL;
	else if ( strstr( cmd, "-horizontal" ) ) direction = HORIZONTAL;
	else if ( strstr( cmd, "-hori" ) ) direction = HORIZONTAL;
	else if ( strstr( cmd, "-hor" ) ) direction = HORIZONTAL;
	return( direction );

}

double ParseForDouble ( DexApparatus *apparatus, const char *cmd, const char *key ) {

	double value;
	int	 values;
	char *ptr;

	if ( !cmd ) return( NaN );
	else if ( ptr = strstr( cmd, key ) ) {
		values = sscanf( ptr + strlen( key ) + 1, "%lf", &value );
		if ( values == 1) return( value );
		else return( NaN );
	}
	else return( NaN );
}

double ParseForFrequency ( DexApparatus *apparatus, const char *cmd ) {
	double frequency;
	if ( !_isnan( frequency = ParseForDouble( apparatus, cmd, "-frequency" ) ) ) return( frequency );
	else return( 1.0 );
}


double ParseForDuration ( DexApparatus *apparatus, const char *cmd ) {
	double duration;
	if ( !_isnan( duration = ParseForDouble( apparatus, cmd, "-duration" ) ) ) return( duration );
	else return( 30.0 );
}

double ParseForPinchForce( DexApparatus *apparatus, const char *cmd ) {
	double pinch;
	if ( !_isnan( pinch = ParseForDouble( apparatus, cmd, "-pinch" ) ) ) return( pinch );
	else return( 0.0 );
}

double ParseForFilterConstant( DexApparatus *apparatus, const char *cmd ) {
	double filter;
	if ( !_isnan( filter = ParseForDouble( apparatus, cmd, "-filter" ) ) ) return( filter );
	else return( 1.0 );
}

#if 0
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
#endif


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
	
	int task = RUN_SUBJECT;
	bool compile = false;
	bool raz = false;
	bool instruct_to_sit = false;
	bool preconfigure = false;

	int return_code;

	HWND work_dlg, mouse_dlg, mass_dlg, camera_dlg;
	
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
	if ( strstr( lpCmdLine, "-pictures"  ) ) task = SHOW_PICTURES;
	if ( strstr( lpCmdLine, "-finish"  ) ) task = MISC_INSTALL;
	if ( strstr( lpCmdLine, "-checkID"  ) ) task = MISC_INSTALL;
	if ( strstr( lpCmdLine, "-audio"  ) ) task = AUDIO_CHECK;

	// Resetting the offsets on the force sensors can be considered as a task in itself 
	//  by specifying -offsets in the command line.
	// Here we also give the opportunity to execute it in addition to another specified task, 
	//  making it available to be run at the start of a simuation session.

	if ( strstr( lpCmdLine, "-raz"  ) ) raz = true;
	if ( strstr( lpCmdLine, "-sit"  ) ) instruct_to_sit = true;
	if ( strstr( lpCmdLine, "-preconfig"  ) ) preconfigure = true;

	// This should invoke the command interpreter on the specified script.
	// For the moment, the interpreter is not working.
	if ( strstr( lpCmdLine, "-script" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-script=" ) ) ) {
			sscanf( ptr + strlen( "-script=" ), "%s", inputScript );
		}
		task = RUN_SCRIPT;	
	}
	if ( strstr( lpCmdLine, "-protocol" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-protocol=" ) ) ) {
			sscanf( ptr + strlen( "-protocol=" ), "%s", inputProtocol );
		}
		task = RUN_PROTOCOL;	
	}
	if ( strstr( lpCmdLine, "-session" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-session=" ) ) ) {
			sscanf( ptr + strlen( "-session=" ), "%s", inputSession );
		}
		task = RUN_SESSION;	
	}
	if ( strstr( lpCmdLine, "-subjects" ) ) {
		char *ptr;
		if ( ( ptr = strstr( lpCmdLine, "-subjects=" ) ) ) {
			sscanf( ptr + strlen( "-subjects=" ), "%s", inputSubject );
		}
		task = RUN_SUBJECT;	
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

		// Create an interface to a remote monitoring station.
		monitor = new DexMonitorServerGUI( targets->nVerticalTargets, targets->nHorizontalTargets, tracker->nCodas );

		// Create a dialog box to emulate selection of the extra masses.
		mass_dlg = DexCreateMassGUI();

		// And the photo camera.
		camera_dlg = DexCreatePhotoCameraGUI();

		// Now put all the pieces together to create a simulator of the DEX hardware.
		apparatus = new DexApparatus( tracker, targets, sounds, adc, monitor, work_dlg, mass_dlg, camera_dlg );

		// How did we last leave the simulator?
		LoadGUIState();
	}

	else apparatus = new DexCompiler( outputScript );



	// Run the initialization for the apparatus (or compiler).
	apparatus->Initialize();

	// Clear the information window.
	apparatus->HideStatus();

	// If we are running the task on the simulator, give the operator a chance to set the initial hardware configutation.
	if ( preconfigure ) {
		return_code = apparatus->WaitSubjectReady( "Desktop-Computer.bmp", "DEX Desktop Simulator\nUse the GUI to set the initial configuration that you want to test." );
		if ( return_code == ABORT_EXIT ) exit( return_code );
		SaveGUIState();
	}

	// If the command line flag was set to do the transducer offset cancellation, then do it before running the specified task.
	// Note, however, that the force offset cancellation can also be done as a separate task by specifying -offsets in the command line.
	if ( raz ) {
		while ( RETRY_EXIT == ( return_code = RunTransducerOffsetCompensation( apparatus, lpCmdLine ) ) );
		if ( return_code == ABORT_EXIT ) exit( return_code );
	}

	if ( instruct_to_sit ) {

		DexSubjectPosture desired_posture = ParseForPosture( lpCmdLine );

		// Instruct subject to take the appropriate position in the apparatus
		//  and wait for confimation that he or she is ready.
		if ( desired_posture == PostureSeated ) {
			return_code = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", "Take a seat, attach the waist and shoulder straps and attach the wrist box to your right wrist.", OkToContinue );
		}
		else if ( desired_posture == PostureSupine ) {
			return_code = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", "Lie down on the supine box, attach the waist, thigh and shoulder straps and attach the wrist box to your right wrist.", OkToContinue );
		}
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
		
	case RUN_PROTOCOL:
		while ( RETRY_EXIT == ( return_code = RunProtocol( apparatus, inputProtocol ) ) );
		break;
		
	case RUN_SESSION:
		while ( RETRY_EXIT == ( return_code = RunSession( apparatus, inputSession ) ) );
		break;
		
	case RUN_SUBJECT:
		while ( RETRY_EXIT == ( return_code = RunSubject( apparatus, inputSubject ) ) );
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

	case AUDIO_CHECK:
		while ( RETRY_EXIT == ( return_code = CheckAudio( apparatus, lpCmdLine ) ) );
		break;

	case MISC_INSTALL:
		while ( RETRY_EXIT == ( return_code = MiscInstall( apparatus, lpCmdLine ) ) );
		break;

	case SHOW_PICTURES:
		while ( RETRY_EXIT == ( return_code = ShowPictures( apparatus, lpCmdLine ) ) );
		break;
	}

	// Record the simulated state of the apparatus, so that the next trial starts from here.
	SaveGUIState();

	// Display the results.
	if ( return_code != ABORT_EXIT && !compile ) {
		// Create a window for plotting the data.
		DexInitPlots();		
		DexPlotData( apparatus );
	}
	
	apparatus->Quit();
	return 0;
}



