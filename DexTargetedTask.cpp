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

// Targeted trial parameters;
// These may be overridden by the -targets= command line argument.
int targetSequence[MAX_SEQUENCE_ENTRIES] = { 5, 2, 11, 5, 11, 2, 5, 8, 5, 11 };	// List of targets for point-to-point movements.
int targetSequenceN = 10;

double targetedMovementTime = 1.0;			// Time to perform each movement.
double targetedMinMovementExtent =  100.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double targetedMaxMovementExtent = 1000.0;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.

/*********************************************************************************/

int PrepTargeted( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	char *target_filename = 0;
	char *mtb, *dsc;
	
	int	direction = ParseForDirection( apparatus, params );
	int eyes = ParseForEyeState( params );
	DexSubjectPosture posture = ParseForPosture( params );

	DexTargetBarConfiguration bar_position;
	Vector3 direction_vector = {0.0, 1.0, 0.0};
	Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	if ( posture == PostureSeated ) {
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", "Seated?   Belts attached?   Wristbox on wrist?%s", OkToContinue );
	}
	else if ( posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", "Lying Down?  Belts attached?  Wristbox on wrist?%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );

	// Prompt the subject to stow the tapping surfaces.
	status = apparatus->fWaitSubjectReady( "Folded.bmp", "Check that tapping surfaces are folded.%s", OkToContinue );
	if ( status == ABORT_EXIT ) exit( status );

	// Cancel any force offsets.
	RunTransducerOffsetCompensation( apparatus, params );

	// Instruct the subject on the task to be done.
	
	GiveDirective( apparatus, "You will first pick up the manipulandum with\nthumb and index finger centered.", "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "TargetedV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "TargetedH.bmp";
	}

	GiveDirective( apparatus, "To start, move to the target that is blinking.", mtb );
	GiveDirective( apparatus, "Move quickly and accurately to each lighted target.", dsc );

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	// These are static so that if the params string does not specify a value,
	//  whatever was used the previous call will be used again.
	static int	direction = VERTICAL;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename;

	// Get the subject posture from the command line.
	posture = ParseForPosture( params );

	// Which mass should be used for this set of trials?
	DexMass mass = ParseForMass( params );

	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// What is the target sequence? If not specified in the command line, use the default.
	if ( target_filename = ParseForTargetFile( params ) ) targetSequenceN = LoadSequence( target_filename, targetSequence, MAX_SEQUENCE_ENTRIES );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepTargeted( apparatus, params );

	// Start acquisition and acquire a baseline.
	apparatus->SignalEvent( "Initiating set of discrete movements." );
	apparatus->StartAcquisition( "TRGT", maxTrialDuration );
	
	// Acquire
	Sleep( 1000 );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

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

		// Allow a fixed time to reach the target.
		apparatus->Wait( targetedMovementTime);

	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	BlinkAll( apparatus );
	BlinkAll( apparatus );
	status = apparatus->WaitSubjectReady( "cradles.bmp", "Trial terminated.\nPlease place the maniplandum in the empty cradle." );

	if ( status == ABORT_EXIT ) exit( status );
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->Wait( 1.0 );

	// Stop collecting data.
	apparatus->ShowStatus( "Retrieving data ...", "working.bmp" );
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ...", "working.bmp" );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occlusions exceed tolerance." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, direction_vector, "Movement amplitude out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// TODO: Are there more post hoc tests to be done here?

	apparatus->HideStatus();
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "ok.bmp", "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}



