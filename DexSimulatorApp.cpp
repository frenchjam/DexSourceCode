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

#include "DexApparatus.h"
#include "Dexterous.h"

/*********************************************************************************/

// #define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/

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
int oscillationUpperTarget = 9;						// Targets showing desired amplitude of cyclic movement.
int oscillationLowerTarget = 3;
int oscillationCenterTarget = 6;
double oscillationTime = 10.0;
double oscillationMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double oscillationMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double oscillationMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
int	oscillationMinCycles = 6;	// Minimum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
int oscillationMaxCycles = 0;	// Maximum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
float cycleHysteresis = 10.0;

// Collision trial parameters;
int collisionInitialTarget = 6;
int collisionUpTarget = 11;
int collisionDownTarget = 1;

#define UP		0
#define DOWN	1
int collisionSequenceN = 10;
int collisionSequence[] = { DOWN, UP, UP, DOWN, DOWN, DOWN, UP, DOWN, UP, UP };
double collisionTime = 2.0;
double collisionMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double collisionMovementThreshold = 10.0;

int exit_status = NORMAL_EXIT;

Vector3 expected_position[2] = {{-1000.0, 0.0, 2500.0}, {0.0, 900.0, 2500.0}};
Quaternion expected_orientation[2];
unsigned int RefMarkerMask = 0x000f0000;

float flashTime = 0.3;

/*********************************************************************************/

int RunInstall( DexApparatus *apparatus ) {

	int status;

	// Express the expected orientation of each fo the CODA units as a quaternion.
	apparatus->SetQuaterniond( expected_orientation[0], 90.0, apparatus->kVector );
	apparatus->SetQuaterniond( expected_orientation[1],  0.0, apparatus->iVector );

	// Check that the 4 reference markers are in the ideal field-of-view of each Coda unit.
	status = apparatus->CheckTrackerFieldOfView( 0, RefMarkerMask, -1000.0, 1000.0, -1000.0, 1000.0, 2000.0, 4000.0 );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	status = apparatus->CheckTrackerFieldOfView( 1, RefMarkerMask, -1000.0, 1000.0, -1000.0, 1000.0, 2000.0, 4000.0 );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform the alignment based on those markers.
	status = apparatus->PerformTrackerAlignment();
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	status = apparatus->CheckTrackerPlacement( 0, 
										expected_position[0], 45.0, 
										expected_orientation[0], 45.0, 
										"Placement error - Coda Unit 0." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	status = apparatus->CheckTrackerPlacement( 1, 
										expected_position[1], 10.0, 
										expected_orientation[1], 10.0, 
										"Placement error - Coda Unit 1." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that the tracker is still aligned.
	status = apparatus->CheckTrackerAlignment( RefMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum on the chair.
	status = apparatus->WaitSubjectReady( "Place maniplandum in specified position on the chair." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->StartAcquisition( 5.0 );
	apparatus->Wait( 5.0 );
	apparatus->StopAcquisition();
	apparatus->SaveAcquisition( "ALGN" );

	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, int direction, int target_sequence[], int n_targets ) {
	
	int status = 0;

	int bar_position;

	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

#ifndef SKIP_PREP

	// Check that the tracker is still aligned.
	status = apparatus->CheckTrackerAlignment( RefMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
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
#endif

	// Start acquiring data.
	apparatus->StartAcquisition( targetedMaxTrialTime );

	// Wait until the subject gets to the target before moving on.
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( target_sequence[0], uprightNullOrientation );
	else status = apparatus->WaitUntilAtHorizontalTarget( target_sequence[0], supineNullOrientation, defaultPositionTolerance, 1.0 ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Make sure that the target is turned back on if a timeout occured.
	// Normally this won't do anything.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) apparatus->VerticalTargetOn( target_sequence[0] );
	else apparatus->HorizontalTargetOn( target_sequence[0] );
		
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

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
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

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
	
	if ( direction == VERTICAL ) status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, apparatus->jVector, NULL );
	else status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, apparatus->kVector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}


/*********************************************************************************/

int RunOscillations( DexApparatus *apparatus ) {
	
	int status = 0;
	
	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[TargetBarRight], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( PostureSeated, TargetBarRight, TappingFolded );
	if ( status == ABORT_EXIT ) exit( status );

	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( "Move to the flashing target.\nWhen 2 new targets appear, make oscillating\nmovements at approx. 1 Hz. " );
	if ( status == ABORT_EXIT ) exit( status );

	// Light up the central target.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationCenterTarget );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( oscillationCenterTarget );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Start acquiring data.
	apparatus->StartAcquisition( oscillationMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationLowerTarget );
	apparatus->TargetOn( oscillationUpperTarget );
	
	// Measure data during oscillations performed over a fixed duration.
	apparatus->Wait( oscillationTime );
	
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "OSCI" );
	
	// Check the quality of the data.
	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementAmplitude( oscillationMaxMovementExtent, oscillationMaxMovementExtent, apparatus->jVector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, apparatus->jVector, cycleHysteresis, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

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
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( collisionInitialTarget );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );
	for ( int target = 0; target < collisionSequenceN; target++ ) {
		
		// Ready to start, so light up starting point target.
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		
		// Allow a fixed time to reach the starting point before we start blinking.
		apparatus->Wait( movementTime );
				
		apparatus->TargetsOff();
		if ( collisionSequence[target] ) {
			apparatus->VerticalTargetOn( collisionUpTarget );
			apparatus->MarkEvent( TRIGGER_MOVE_UP);
		}
		else {
			apparatus->VerticalTargetOn( collisionDownTarget );
			apparatus->MarkEvent( TRIGGER_MOVE_DOWN );
		}

		// Allow a fixed time to reach the target before we start blinking.
		apparatus->Wait( flashTime );
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		apparatus->Wait( movementTime - flashTime );

		
	}
	
	apparatus->TargetsOff();

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "COLL" );

	// Check if trial was completed as instructed.
	status = apparatus->CheckMovementDirection( 1, 0.0, 1.0, 0.0, collisionMovementThreshold );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
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
	if ( strstr( lpCmdLine, "-rt"     ) ) apparatus_type = DEX_RTNET_APPARATUS;
	if ( strstr( lpCmdLine, "-osc"    ) ) protocol = OSCILLATION_PROTOCOL;
	if ( strstr( lpCmdLine, "-coll"   ) ) protocol = COLLISION_PROTOCOL;
	if ( strstr( lpCmdLine, "-script" ) ) protocol = RUN_SCRIPT;
	if ( strstr( lpCmdLine, "-calib"  ) ) protocol = CALIBRATE_TARGETS;
	if ( strstr( lpCmdLine, "-install"  ) ) protocol = INSTALL_PROCEDURE;
	
	switch ( apparatus_type ) {
		
	case DEX_MOUSE_APPARATUS:
		apparatus = new DexMouseApparatus( hInstance );
		MessageBox( NULL, "Set desired startup state.", "DexSimulatorApp", MB_OK );
		break;
		
	case DEX_CODA_APPARATUS:
		apparatus = new DexCodaApparatus();
		break;

  case DEX_RTNET_APPARATUS:
		apparatus = new DexRTnetApparatus();
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
		while ( RETRY_EXIT == RunOscillations( apparatus ) );
		break;
		
	case COLLISION_PROTOCOL:
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

	case INSTALL_PROCEDURE:
		while ( RETRY_EXIT == RunInstall( apparatus ) );
		break;

		
	}
	
	apparatus->Quit();
	return 0;
}



