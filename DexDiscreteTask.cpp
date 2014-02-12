/*********************************************************************************/
/*                                                                               */
/*                             DexDiscreteTask.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of point-to-point between just two targets, but with random timing.
 */

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include <VectorsMixin.h>
#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

//#define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/


// Targeted trial parameters;
int delaySequence[] = { 1, 1.5, 1, 2, 1.5, 3, 1, 2, 1 };	// Delays between the discrete movements.
int delaySequenceN = sizeof( delaySequence ) / sizeof( *delaySequence );
int discreteTargets[2] = { 3, 7};

int discreteFalseStartTolerance = 5;
double discreteFalseStartThreshold = 1.0;
double discreteFalseStartHoldTime = 0.250;
double discreteFalseStartFilterConstant = 1.0;

double discreteMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double discreteMaxMovementExtent = 200.0;	// Maximum amplitude along the movement direction (Y). Set to 1g.0 to simulate error.
double discreteCycleHysteresis = 10.0;	// Parameter used to adjust the detection of cycles. 

/*********************************************************************************/

int RunDiscrete( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	// These are static so that if the params string does not specify a value,
	//  whatever was used the previous call will be used again.
	static int	direction = VERTICAL;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	// Which mass should be used for this set of trials?
	DexMass mass = ParseForMass( params );

	direction = ParseForDirection( apparatus, params, posture, bar_position, direction_vector, desired_orientation );
	static int eyes = ParseForEyeState( params );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	apparatus->ShowStatus( "Hardware configuration check ..." );
//	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Folded.bmp", "Check that tapping surfaces are folded.\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Belts2.bmp", "Seated?   Belts attached?   Wristbox on wrist?\n\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
//	status = apparatus->SelectAndCheckMass( mass );
//	if ( status == ABORT_EXIT ) exit( status );

	apparatus->ShowStatus( "Starting set of targeted trials ..." );
	// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "InHand.bmp", "Hold the manipulandum with thumb and \nindexfinger centered. \nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );
   
	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n Or \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Wait_discrete.bmp", "Align the manipulandum with the flashing target. \nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquiring data.
	apparatus->StartAcquisition( "TRGT", maxTrialDuration );

	// Wait until the subject gets to the target before moving on.
	char *wait_at_target_message = "Too long to reach desired target.";
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( discreteTargets[0] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, wait_at_target_message );
	else status = apparatus->WaitUntilAtHorizontalTarget( discreteTargets[0] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, wait_at_target_message ); 
	if ( status == ABORT_EXIT ) exit( status );
	// Light up the next target, alternating between the two.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( discreteTargets[0] );
		apparatus->VerticalTargetOn( discreteTargets[1] );
	}
	else {
		apparatus->HorizontalTargetOn( discreteTargets[0] );
		apparatus->HorizontalTargetOn( discreteTargets[1] );
	}
		
	if ( eyes == CLOSED ) apparatus->WaitSubjectReady("Discrete.bmp", "Close your eyes and move the manipulandum \n to the opposite target at beep.\nPress OK when ready to continue." );
	else apparatus->WaitSubjectReady("Discrete.bmp", "Open eyes and move the manipulandum \n to the opposite target at beep.\nPress <OK> when ready to continue." );
	

	if ( status == ABORT_EXIT ) exit( status );
	// Collect baseline data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Step through the list of targets.
	for ( int delay = 1; delay < delaySequenceN; delay++ ) {
		
		// Make a beep.
		apparatus->MarkEvent( TRIGGER_MOVEMENT );	
		apparatus->Beep();
		
		// Allow a random time before triggering the next target.
		// Takes into account the duration of the beep.
		apparatus->Wait( delaySequence[delay] - beepDuration );
		
	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Let the subject know that they are done.
	BlinkAll( apparatus );

	// Stop collecting data.
	apparatus->ShowStatus( "Retrieving data ..." );
	apparatus->StopAcquisition();
	
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ..." );

	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckMovementAmplitude( discreteMinMovementExtent, discreteMaxMovementExtent, direction_vector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement. 
	// We expect as many as there are items in the sequence. 
	// We accept if there are a few less.
	status = apparatus->CheckMovementCycles( delaySequenceN / 2 - 2, delaySequenceN, direction_vector, discreteCycleHysteresis, "Not as many movements as we expected.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	status = apparatus->CheckEarlyStarts( discreteFalseStartTolerance, discreteFalseStartHoldTime, 
		discreteFalseStartThreshold, discreteFalseStartFilterConstant, 
		"Too many early starts.\nPlease wait for the beep each time.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}



