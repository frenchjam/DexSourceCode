/*********************************************************************************/
/*                                                                               */
/*                             DexTargetedTask.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of point-to-point target movements to a sequence of targets.
 */

/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

// #define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/


// Targeted trial parameters;
int targetSequence[] = { 2, 5, 2, 11, 8, 5, 2, 11, 2, 8,
						 2, 11, 8, 11, 5, 8, 5, 2, 8, 11,
						 2, 8, 2, 11, 5, 8, 2, 8,  2,  5,
						 11, 2, 11, 2, 11, 5, 8, 5, 11, 2,
						 8, 2, 11, 2, 8, 5, 11, 2, 11, 2,
						 11, 8, 11, 5, 8, 2, 11, 11, 9, 12
						};	// List of targets for point-to-point movements.
int targetSequenceN = 40;

double targetedMovementTime = 1.0;			// Time to perform each movement.
double targetedMinMovementExtent = 10.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double targetedMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	static int	direction = VERTICAL;
	static int bar_position = TargetBarRight;
	static int posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	direction = ParseForDirection( apparatus, params, posture, bar_position, direction_vector, desired_orientation );

#ifndef SKIP_PREP

	ShowStatus( apparatus, "Initiating hardware configuration phase ..." );

	// Tell the subject which configuration should be used.
	// TODO: Move this into a protocol, rather than here in a task, because the subject is likely to do multiple
	//   blocks of trials in the same configuration.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position. Place the Target Bar in the %s position. Place the tapping surfaces in the %s position. Press <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[bar_position], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the tracker is still aligned.
	ShowStatus( apparatus, "Tracker alignment check ..." );
	status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Take the seat and attach the belts and the wrist box. Press OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	ShowStatus( apparatus, "Hardware configuration complete ..." );

#endif

	apparatus->Comment( "Starting set of targeted trials." );
	// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Pick up the manipulandum in the right hand. Be sure that thumb and forefinger are centered.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Grip not centered or not in hand." );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquiring data.
	apparatus->StartAcquisition( "TRGT", maxTrialDuration );

	// Wait until the subject gets to the target before moving on.
	char *wait_at_target_message = "Too long to reach desired target.";
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( targetSequence[0], desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, wait_at_target_message );
	else status = apparatus->WaitUntilAtHorizontalTarget( targetSequence[0], desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, wait_at_target_message ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Make sure that the target is turned back on if a timeout occured.
	// Normally this won't do anything.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) apparatus->VerticalTargetOn( targetSequence[0] );
	else apparatus->HorizontalTargetOn( targetSequence[0] );

	// Collect basline data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Step through the list of targets.
	for ( int target = 1; target < targetSequenceN; target++ ) {
		
		// Light up the next target, and mark the event for post hoc analysis.
		apparatus->TargetsOff();
		apparatus->MarkEvent( TRIGGER_MOVEMENT );
		if ( direction == VERTICAL ) apparatus->VerticalTargetOn( targetSequence[ target ] );
		else apparatus->HorizontalTargetOn( targetSequence[ target ] ); 


		// Make a beep. Here we test the tones and volumes.
		apparatus->SoundOn( target % apparatus->nTones, 1 );
		apparatus->Wait( beepDuration );
		apparatus->SoundOff();
		
		// Allow a fixed time to reach the target.
		// Takes into account the duration of the beep.
		apparatus->Wait( targetedMovementTime - beepDuration );
		
	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Let the subject know that they are done.
	BlinkAll( apparatus );

	// Stop collecting data.
	ShowStatus( apparatus, "Retrieving data ..." );
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	ShowStatus( apparatus, "Checking data ..." );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occlusions exceed tolerance." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, direction_vector, "Movement amplitude out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	HideStatus();
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}



