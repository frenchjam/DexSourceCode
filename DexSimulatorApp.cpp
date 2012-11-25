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

//#include "stdafx.h"
#include "resource.h"

#include <Winsock2.h>

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <useful.h>
#include <screen.h>
#include <3dMatrix.h>
#include <DexTimers.h>
#include "ConfigParser.h" 

#include <gl/gl.h>
#include <gl/glu.h>

#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

#include "AfdObjects.h"
#include "CodaObjects.h"
#include "DexGlObjects.h"
#include "DexApparatus.h"
#include "Dexterous.h"

#include <Views.h>
#include <Layouts.h>

int exit_status = NORMAL_EXIT;

int RunTargeted( DexApparatus *apparatus, DexTargetBarConfiguration bar_position, int target_sequence[], int n_targets  );
int RunOscillations( DexApparatus *apparatus );
int RunCollisions( DexApparatus *apparatus );
int RunScript( DexApparatus *apparatus, const char *filename );

/*********************************************************************************/

// Common paramters.
double baselineTime = 1.0;					// Duration of pause at first target before starting movement.
double continuousDropoutTimeLimit = 0.050;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double cumulativeDropoutTimeLimit = 1.000;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
int desired_posture = PostureSeated;
double beepTime = BEEP_DURATION;

// Targeted trial parameters;
int targetSequence[] = { 0, 1, 0, 4, 0, 2, 0, 3, 0 };	// List of targets for point-to-point movements.
int targetSequenceN = 9;
double movementTime = 2.0;					// Time allowed for each movement.
double targetedMaxTrialTime = 120.0;		// Max time to perform the whole list of movements. Set to 12 to simulate error.
double targetedMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double targetedMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.

// Oscillation trial parameters.
int upper_target = 3;						// Targets showing desired amplitude of cyclic movement.
int lower_target = 1;
int center_target = 2;
double oscillationTime = 10.0;
double oscillationMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.

// Collision trial parameters;
int collisionInitialTarget = 2;
int collisionSequenceN = 10;
double collisionTime = 2.0;
double collisionMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, int direction, int target_sequence[], int n_targets ) {
	
	int status = 0;

	int bar_position;

	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	
	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[bar_position], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( PostureSeated, bar_position, TappingFolded );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Send information about the actual configuration to the ground.
	// This is redundant, because the SelectAndCheckConfiguration() command will do this as well.
	// But I want to demonstrate that it can be used independent from the check as well.
	apparatus->SignalConfiguration();
	
	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Take a seat and attach the belts.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Wait until the subject gets to the target before moving on.
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( target_sequence[0] );
	else status = apparatus->WaitUntilAtHorizontalTarget( target_sequence[0] ); 
	if ( status == ABORT_EXIT ) exit( status );
	
	// Start acquiring data.
	apparatus->StartAcquisition( targetedMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Step through the list of targets.
	for ( int target = 1; target < n_targets; target++ ) {
		
		// Light up the next target.
		apparatus->TargetsOff();
		if ( direction == VERTICAL ) apparatus->VerticalTargetOn( target_sequence[ target ] );
		else apparatus->HorizontalTargetOn( target_sequence[ target ] ); 


		// Make a beep. Here we test the tones and volumes.
		apparatus->SoundOn( target, target );
		apparatus->Wait( beepTime );
		apparatus->SoundOff();
		
		// Allow a fixed time to reach the target.
		// Takes into account the duration of the beep.
		apparatus->Wait( movementTime - beepTime );
		
	}
	
	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Stop collecting data.
	apparatus->StopAcquisition();
	
	// Save the data.
	apparatus->SaveAcquisition( "TRGT" );
	
	// Check the quality of the data.
	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	if ( direction == VERTICAL ) status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, 0.0, 1.0, 0.0, NULL );
	else status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, 0.0, 0.0, 1.0, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}


/*********************************************************************************/

int RunOscillations( DexApparatus *apparatus ) {
	
	int status = 0;
	
	// Light up the central target.
	apparatus->TargetsOff();
	apparatus->TargetOn( center_target );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtTarget( 2 );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Start acquiring data.
	apparatus->StartAcquisition( oscillationMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	apparatus->TargetOn( lower_target );
	apparatus->TargetOn( upper_target );
	
	// Measure data during oscillations performed over a fixed duration.
	apparatus->Wait( oscillationTime );
	
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "OSCI" );
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	exit_status = NORMAL_EXIT;
	
	return( exit_status );
	
}

/*********************************************************************************/

int RunCollisions( DexApparatus *apparatus ) {
	
	int status = 0;
	
	Sleep( 1000 );
	apparatus->StartAcquisition( collisionMaxTrialTime );
	
	for ( int target = 0; target < collisionSequenceN; target++ ) {
		
		// Ready to start, so light up starting point target.
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		
		// Allow a fixed time to reach the starting point before we start blinking.
		apparatus->Wait( movementTime );
		
		// Now wait until the subject gets to the target before moving on.
		status = apparatus->WaitUntilAtTarget( collisionInitialTarget );
		if ( status == IDABORT ) exit( ABORT_EXIT );
		
		apparatus->TargetsOff();
		
		// Add bee bop
		// check amplitude after
		// count cycles
		
		// Allow a fixed time to reach the target before we start blinking.
		apparatus->Wait( movementTime );
		
	}
	
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "COLL" );
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( exit_status );
	
}
/*********************************************************************************/

int RunTargetCalibration( DexApparatus *apparatus ) {
	int exit_status;
	exit_status = apparatus->CalibrateTargets();
	return( exit_status );
}

/*********************************************************************************/

/* 
* The DexCompiler apparatus generates a script of the top-level commands.
* Here we run the set of steps defined by such a script, instead of running 
* a protocol defined by a C subroutine (as above).
*/

//  !!!!!!!!!!!!!!!!!!!! THIS IS NOT UP TO DATE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   It may not cover all the required commands.

int RunScript( DexApparatus *apparatus, const char *filename ) {
	
	FILE *fp;
	int status = 0;
	char line[1024];
	
	fp = fopen( filename, "r" );
	if ( !fp ) {
		char message[1024];
		sprintf( message, "Error opening script file:\n  %s", filename );
		MessageBox( NULL, message, "DEX Error", MB_OK );
		return( ERROR_EXIT );
	}
	
	while ( fgets( line, sizeof( line ), fp ) ) {
		
		char token[1024];
		
		sscanf( line, "%s", token );
		
		if ( !strcmp( token, "SelectAndCheckConfiguration" ) ) {
			
			int posture, target_config, tapping_config;
			sscanf( line, "%s %d %d %d", token, &posture, &target_config, &tapping_config );
			
			// Select which configuration of the hardware should be used.
			// Verify that it is in the correct configuration, and if not, 
			//  give instructions to the subject about what to do.
			status = apparatus->SelectAndCheckConfiguration( posture, target_config, tapping_config );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
		
		if ( !strcmp( token, "WaitSubjectReady" ) ) {
			
			char *prompt = strpbrk( line, " \t" );
			if ( !prompt ) prompt = "Ready?";
			else prompt++;
			
			// Instruct subject to take the appropriate position in the apparatus
			//  and wait for confimation that he or she is ready.
			status = apparatus->WaitSubjectReady( prompt );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
		
		if ( !strcmp( token, "WaitUntilAtTarget" ) ) {
			
			int target = 0;
			sscanf( line, "%s %d", token, &target );
			
			// Wait until the subject gets to the target before moving on.
			status = apparatus->WaitUntilAtTarget( target );
			if ( status == ABORT_EXIT ) exit( status );
		}
		
		if ( !strcmp( token, "StartAcquisition" ) ) {
			float max_duration = 120.0;
			sscanf( line, "%s %f", token, &max_duration );
			// Start acquiring data.
			apparatus->StartAcquisition( max_duration );
		}
		
		if ( !strcmp( token, "Wait" ) ) {
			
			double duration = 0.0;
			sscanf( line, "%s %lf", token, &duration );
			
			// Collect one second of data while holding at the starting position.
			apparatus->Wait( duration );
		}
		
		if ( !strcmp( token, "TargetsOff" ) ) {
			apparatus->TargetsOff();
		}
		
		if ( !strcmp( token, "TargetOn" ) ) {
			
			int target = 0;
			sscanf( line, "%s %d", token, &target );
			// Light up the next target.
			apparatus->TargetOn( target );
			
		}
		
		if ( !strcmp( token, "StopAcquisition" ) ) {
			// Stop collecting data.
			apparatus->StopAcquisition();
		}
		
		if ( !strcmp( token, "SaveAcquisition" ) ) {
			char tag[256];
			sscanf( line, "%s %s", token, tag );
			// Save the data.
			apparatus->SaveAcquisition( tag );
		}
		
		if ( !strcmp( token, "CheckVisibility" ) ) {
			
			double cumulative, continuous;
			sscanf( line, "%s %lf %lf", token, &cumulative, &continuous );
			
			// Check the quality of the data.
			status = apparatus->CheckVisibility( cumulative, continuous, NULL );
			if ( status == ABORT_EXIT ) exit( status );
			if ( status == RETRY_EXIT ) return( status );
		}
		
		if ( !strcmp( token, "SignalNormalCompletion" ) ) {
			
			char *prompt = strpbrk( line, " \t" );
			if ( !prompt ) prompt = "Terminated normally.";
			
			// Indicate to the subject that they are done.
			status = apparatus->SignalNormalCompletion( prompt );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
	}
	
	return( NORMAL_EXIT );
	
}

/**************************************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	
	DexApparatus *apparatus;
	DexTimer session_timer;
	
	int apparatus_type = DEX_MOUSE_APPARATUS;
	int protocol = TARGETED_PROTOCOL;
	char *script = "DexSampleScript.dex";

	int return_code;
	
	// Parse command line.
	
	if ( strstr( lpCmdLine, "-coda"   ) ) apparatus_type = DEX_CODA_APPARATUS;
	if ( strstr( lpCmdLine, "-osc"    ) ) protocol = OSCILLATION_PROTOCOL;
	if ( strstr( lpCmdLine, "-coll"   ) ) protocol = COLLISION_PROTOCOL;
	if ( strstr( lpCmdLine, "-script" ) ) protocol = RUN_SCRIPT;
	if ( strstr( lpCmdLine, "-calib"  ) ) protocol = CALIBRATE_TARGETS;
	
	
	switch ( apparatus_type ) {
		
	case DEX_MOUSE_APPARATUS:
		apparatus = new DexMouseApparatus( hInstance );
		break;
		
	case DEX_CODA_APPARATUS:
		apparatus = new DexCodaApparatus();
		break;
		
	case DEX_COMPILER:
		apparatus = new DexCompiler();
		break;
		
	}
	
	// Send information about the actual configuration to the ground.
	apparatus->SignalConfiguration();
	
	/*
	* Keep track of the elapsed time from the start of the first trial.
	*/
	DexTimerStart( session_timer );
	
	/*
	* Run one of the protocols.
	*/
	
	switch ( protocol ) {

	case CALIBRATE_TARGETS:
		while ( RETRY_EXIT == RunTargetCalibration( apparatus ) );
		break;
		
	case RUN_SCRIPT:
		while ( RETRY_EXIT == RunScript( apparatus, script ) );
		break;
		
	case OSCILLATION_PROTOCOL:
//		if ( apparatus->type == DEX_VIRTUAL_APPARATUS ) ((DexVirtualTracker *) apparatus->tracker)->SetMovementType( OSCILLATION_PROTOCOL ); 
		while ( RETRY_EXIT == RunOscillations( apparatus ) );
		break;
		
	case COLLISION_PROTOCOL:
//		if ( apparatus->type == DEX_VIRTUAL_APPARATUS ) ((DexVirtualTracker *) apparatus->tracker)->SetMovementType( COLLISION_PROTOCOL ); 
		while ( RETRY_EXIT == RunCollisions( apparatus ) );
		break;
		
	case TARGETED_PROTOCOL:
		do {
			return_code = RunTargeted( apparatus, VERTICAL, targetSequence, targetSequenceN );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		do {
			return_code = RunTargeted( apparatus, HORIZONTAL, targetSequence, targetSequenceN );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		break;
		
	}
	
	apparatus->Quit();
	return 0;
}



