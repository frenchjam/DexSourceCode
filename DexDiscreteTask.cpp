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
double	delaySequence[1024] = { 1, 1.5, 1, 2, 1.5, 3, 1, 2, 1 };	// Delays between the discrete movements.
int		delaySequenceN = 9;
int		discreteTargets[3] = {3, 5, 7};

int discreteFalseStartTolerance = 5;
double discreteFalseStartThreshold = 1.0;
double discreteFalseStartHoldTime = 0.250;
double discreteFalseStartFilterConstant = 1.0;

double discreteMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double discreteMaxMovementExtent = 200.0;	// Maximum amplitude along the movement direction (Y). Set to 1g.0 to simulate error.
double discreteCycleHysteresis = 10.0;	// Parameter used to adjust the detection of cycles. 
Vector3	discreteMovementDirection = {0.0, 1.0, 0.0};	// Movements are nominally in the vertical direction. Could change at some point, I suppose.

/*********************************************************************************/

int PrepDiscrete( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	char *target_filename = 0;
	char *mtb, *dsc;
	
	// What conditions are we about to use.
	DexTargetBarConfiguration bar_position;
	DexSubjectPosture posture = ParseForPosture( params );
	int eyes = ParseForEyeState( params );
	int	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// Prompt the subject to put the target bar in the correct position.
	if ( bar_position == TargetBarRight ) {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "Folded.bmp" : "Folded.bmp" ), 
			"Place the target bar in the right position with tapping surfaces folded.%s", OkToContinue );
	}
	else {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "BarLeft.bmp" : "BarLeft.bmp" ), 
			"Place the target bar in the left position.%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );


	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	if ( posture == PostureSeated ) {
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", MsgQueryReadySeated, OkToContinue );
	}
	else if ( posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", MsgQueryReadySupine, OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );

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
	AddDirective( apparatus, InstructPickUpManipulandum, "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "DiscreteV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "DiscreteH.bmp";
	}

	if ( eyes == OPEN )	{
		AddDirective( apparatus, "To start, you will move to the target that is blinking.", mtb );
		AddDirective( apparatus, "On each beep, you will move quickly and accurately to the other lit target.", dsc );
		AddDirective( apparatus, "Wait for each beep. Stop at each target. Keep your eyes OPEN.", "DiscreteV.bmp" );
	}
	else {
		AddDirective( apparatus, "To start, you will move to the target that is blinking, then CLOSE your eyes.", mtb );
		AddDirective( apparatus, "On each beep, move quickly and accurately to the other (remembered) target location.", dsc );
		AddDirective( apparatus, "Remember to WAIT for each beep, STOP at each target and keep your eyes CLOSED.", "DiscreteH.bmp" );
	}
	ShowDirectives( apparatus );

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
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename = 0;
	char *delay_filename = 0;
	char tag[8] = "Ds";			// D is for discrete.

	fprintf( stderr, "     RunDiscrete: %s\n", params );

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Seated or supine?
	posture = ParseForPosture( params );
	if ( posture == PostureSeated ) strcat( tag, "Up" ); // Up is for upright (seated).
	else strcat( tag, "Su" ); // Su is for supine.

	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) {
		bar_position = TargetBarRight;
		apparatus->CopyVector( discreteMovementDirection, apparatus->jVector );
		strcat( tag, "Ve" );
	}
	else {
		bar_position = TargetBarLeft;
		apparatus->CopyVector( discreteMovementDirection, apparatus->kVector );
		strcat( tag, "Ho" );
	}

	// Eyes open or closed?
	eyes = ParseForEyeState( params );
	if ( eyes == OPEN ) strcat( tag, "Op" ); 
	else strcat( tag, "Cl" ); 

	// What is the sequence of delays? If not specified in the command line, use the default.
	if ( delay_filename = ParseForDelayFile( params ) ) delaySequenceN = LoadSequence( delaySequence, delay_filename );

	// What are the limits of each discrete movement?
	if ( target_filename = ParseForRangeFile( params ) ) LoadTargetRange( discreteTargets, target_filename );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepDiscrete( apparatus, params );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	else {
		status = CheckInstall( apparatus, posture, bar_position );
		if ( status != NORMAL_EXIT ) return( status );
	}

	// Indicate to the subject that we are ready to start and wait for their go signal.
	status = apparatus->WaitSubjectReady( "ReadyToStart.bmp", MsgReadyToStart );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquisition and acquire a baseline.
	// Presumably the manipulandum is not in the hand. 
	// It should have been left either in a cradle or the retainer at the end of the last action.
	apparatus->SignalEvent( "Initiating set of discrete movements." );
	apparatus->StartFilming( tag );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );
	
	// Instruct subject to take the specified mass.
	// If the correct mass is already on the manipulandum and out of the cradle, 
	//  this will move right on to the next step.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	apparatus->ShowStatus( MsgCheckGripCentered, "working.bmp" );
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, MsgGripNotCentered, "alert.bmp" );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->ShowStatus( "Trial started ...", "working.bmp" );

	// Wait until the subject gets to the target before moving on.
	apparatus->ShowStatus( MsgMoveToBlinkingTarget, "working.bmp" );
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( discreteTargets[LOWER] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget );
	else status = apparatus->WaitUntilAtHorizontalTarget( discreteTargets[LOWER] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Collect some data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	// Light up the pair of targets.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( discreteTargets[LOWER] );
		apparatus->VerticalTargetOn( discreteTargets[UPPER] );
	}
	else {
		apparatus->HorizontalTargetOn( discreteTargets[LOWER] );
		apparatus->HorizontalTargetOn( discreteTargets[UPPER] );
	}
			
	// Indicate what to do next.
	if ( eyes == OPEN ) apparatus->ShowStatus( "Start point-to-point movements. Wait for each beep. Stop at each target. Keep your eyes OPEN.", "working.bmp" ); 
	else apparatus->ShowStatus( "Start point-to-point movements. Wait for each beep. Stop at each target. Keep your eyes CLOSED.", "working.bmp" ); 

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Wait a little to give the subject time to react, in case they were looking at the screen.
	apparatus->Wait( baselineDuration );

	// Step through the list of delays.
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

	// Collect one final bit of data.
	apparatus->Wait( baselineDuration );
	
	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	BlinkAll( apparatus );
	BlinkAll( apparatus );
	status = apparatus->WaitSubjectReady( "PlaceMass.bmp", "Trial terminated.\nPlease place the maniplandum in the empty cradle." );
	
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );
	
	// Stop acquiring.
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	apparatus->SignalEvent( "Acquisition terminated." );
	apparatus->StopFilming();
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ...", "wait.bmp" );
	int n_post_hoc_steps = 4;
	int post_hoc_step = 0;

	// Was the manipulandum obscured?
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occlusions exceed tolerance." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Check that we got a reasonable amount of movement.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for movement ..." );
	status = apparatus->CheckMovementAmplitude( discreteMinMovementExtent, discreteMaxMovementExtent, discreteMovementDirection, "Movement amplitude out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable number of movements. 
	// We expect as many as there are items in the sequence. 
	// But this post hoc test was designed for the oscillations protocol. 
	// So we will have half as many cycles as the number of expected movements.
	int tolerance = 5;
	int fewest = delaySequenceN / 2 - tolerance;
	if (fewest < 1) fewest = 1;
	int most = delaySequenceN / 2 + tolerance;
	if ( most < 1 ) most = 1;

	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for number of movements ..." );
	status = apparatus->CheckMovementCycles( fewest, most, discreteMovementDirection, discreteCycleHysteresis, "Not as many movements as we expected. Would you like to try again?" );
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



