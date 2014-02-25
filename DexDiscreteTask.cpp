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

int PrepDiscrete( DexApparatus *apparatus, const char *params ) {

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
	
	// Show them the targets that will be used.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( discreteTargets[0] );
		apparatus->VerticalTargetOn( discreteTargets[1] );
	}
	else {
		apparatus->HorizontalTargetOn( discreteTargets[0] );
		apparatus->HorizontalTargetOn( discreteTargets[1] );
	}

	// Describe how to do the task, according to the desired conditions.
	GiveDirective( apparatus, "You will first pick up the manipulandum with\nthumb and index finger centered.", "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "DiscreteV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "DiscreteH.bmp";
	}

	if ( eyes == OPEN )	{
		GiveDirective( apparatus, "To start, move to the target that is blinking.", mtb );
		GiveDirective( apparatus, "On each beep,move quickly and accurately to the other\nlit target. Keep your eyes OPEN the entire time.", dsc );
	}
	else {
		GiveDirective( apparatus, "To start, move to the target that is blinking.\nThen CLOSE your eyes.", mtb );
		GiveDirective( apparatus, "On each beep,move quickly and accurately\nto the remebered location of the other target.", dsc );
	}

	return( NORMAL_EXIT );
}


/*********************************************************************************/

int RunDiscrete( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	// These are static so that if the params string does not specify a value,
	//  whatever was used the previous call will be used again.
	static int	direction = VERTICAL;
	static int	eyes = OPEN;
	static DexMass mass = MassMedium;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename = 0;
	char *delay_filename = 0;

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// Eyes open or closed?
	eyes = ParseForEyeState( params );

	// What is the sequence of delays? If not specified in the command line, use the default.
	if ( delay_filename = ParseForDelayFile( params ) ) delaySequenceN = LoadSequence( delay_filename, delaySequence, MAX_SEQUENCE_ENTRIES );
	if ( target_filename = ParseForTargetFile( params ) ) LoadSequence( target_filename, discreteTargets, 2 );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status == ABORT_EXIT ) return( status );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepDiscrete( apparatus, params );

	// Start acquisition and acquire a baseline.
	apparatus->SignalEvent( "Initiating set of discrete movements." );
	apparatus->StartAcquisition( "DISC", maxTrialDuration );
	
	// Acquire
	Sleep( 1000 );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );
   
	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n Or \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->ShowStatus( "Trial started ...", "working.bmp" );

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
		
//	if ( eyes == CLOSED ) apparatus->WaitSubjectReady("Discrete.bmp", "Close your eyes and move the manipulandum \n to the opposite target at beep.\nPress OK when ready to continue." );
//	else apparatus->WaitSubjectReady("Discrete.bmp", "Open eyes and move the manipulandum \n to the opposite target at beep.\nPress <OK> when ready to continue." );
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
	
	// We're done.
	apparatus->TargetsOff();

	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	BlinkAll( apparatus );
	BlinkAll( apparatus );
	status = apparatus->WaitSubjectReady( "cradles.bmp", "Trial terminated.\nPlease place the maniplandum in the empty cradle." );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->Wait( 1.0 );

	// Stop acquiring.
	apparatus->StopAcquisition();
	// Signal to subject that the task is complete.
	apparatus->SignalEvent( "Acquisition terminated." );
	
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ...", "working.bmp" );
	int n_post_hoc_steps = 4;
	int post_hoc_step = 0;

	// Was the manipulandum obscured?
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Check that we got a reasonable amount of movement.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for movement ..." );
	status = apparatus->CheckMovementAmplitude( discreteMinMovementExtent, discreteMaxMovementExtent, direction_vector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable number of movements. 
	// We expect as many as there are items in the sequence. 
	// We accept if there are a few less.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for nubmer of movement ..." );
	status = apparatus->CheckMovementCycles( delaySequenceN / 2 - 2, delaySequenceN, direction_vector, discreteCycleHysteresis, "Not as many movements as we expected.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Did the subject anticipate the starting signal too often?
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for early starts ..." );
	status = apparatus->CheckEarlyStarts( discreteFalseStartTolerance, discreteFalseStartHoldTime, 
		discreteFalseStartThreshold, discreteFalseStartFilterConstant, 
		"Too many early starts.\nPlease wait for the beep each time.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Post hoc tests completed." );

	// Indicate to the subject that they are done.
	// The first NULL parameter says to use the default picture.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}



