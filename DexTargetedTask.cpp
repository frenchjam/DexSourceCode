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
double targetedMinMovementExtent =  20.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double targetedMaxMovementExtent = 1000.0;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
Vector3 targetedMovementDirection = {0.0, 1.0, 0.0};
Quaternion targetedMovementOrientation = {0.0, 0.0, 0.0, 1.0};

int lit_targets[DEX_MAX_TARGETS];

/*********************************************************************************/

int PrepTargeted( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	char *target_filename = 0;
	char *mtb, *dsc;
	
	int	direction = ParseForDirection( apparatus, params );
	int eyes = ParseForEyeState( params );
	DexSubjectPosture posture = ParseForPosture( params );
	DexTargetBarConfiguration bar_position;

	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// Prompt the subject to put the target bar in the correct position.
	// This is no longer able to handle the supine position, but since we are 
	//  not currently planning to do this task in supine, it's OK.
	if ( bar_position == TargetBarRight ) {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "Folded.bmp" : "Folded.bmp" ), 
			"Place the target bar in the right-hand position (Socket 'J') with tapping surfaces closed.%s", OkToContinue );
	}
	else {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "BarLeft.bmp" : "BarLeft.bmp" ), 
			"Place the target bar in the left-hand position with tapping surfaces closed.%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );


	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Show the possible targets.
	for ( int tgt = 0; tgt < DEX_MAX_TARGETS; tgt++ ) lit_targets[tgt] = 0;
	apparatus->TargetsOff();
	for ( tgt = 0; tgt < targetSequenceN; tgt++ ) {
		if ( direction == VERTICAL ) apparatus->VerticalTargetOn( targetSequence[tgt] );
		else apparatus->HorizontalTargetOn( tgt );
		lit_targets[targetSequence[tgt]] = 1;
	}
	int lit_target_count = 0;
	for ( tgt = 0; tgt < DEX_MAX_TARGETS; tgt++ ) if ( lit_targets[tgt] ) lit_target_count++;

	// Instruct the subject on the task to be done.
	AddDirective( apparatus, InstructPickUpManipulandum, "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "TargetedV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "TargetedH.bmp";
	}

	AddDirective( apparatus, "You will place the manipulandum to the right of the blinking target.", mtb );
	AddDirective( apparatus, "You will move quickly and accurately to the right of each lighted target.", dsc );
	char msg[512];
	sprintf( msg, "Targets will be one of the %d currently lit targets.", lit_target_count );
	AddDirective( apparatus, msg, dsc );
	AddDirective( apparatus, "Be sure to hold the manipuladum upright and to the right of each target.", mtb );

	ShowDirectives( apparatus );
	apparatus->TargetsOff();

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	// These are static so that if the params string does not specify a value,
	//  whatever was used the previous call will be used again.
	static int	direction = VERTICAL;
	static int	eyes = OPEN;
	static DexMass mass = MassMedium;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;

	char *target_filename;

	fprintf( stderr, "     RunTargeted: %s\n", params );
	 
	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) {
		bar_position = TargetBarRight;
		apparatus->CopyVector( targetedMovementDirection, apparatus->jVector );
	}
	else {
		bar_position = TargetBarLeft;
		apparatus->CopyVector( targetedMovementDirection, apparatus->kVector );
	}

	// Seated or supine?
	posture = ParseForPosture( params );

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Construct the results filename tag.
	char tag[32];
	strcpy( tag, ParseForTag( params ) );
	strcat( tag, "T" ); // T for targeted.
	if ( direction == VERTICAL ) strcat( tag, "V" );
	else strcat( tag, "H" );
	if ( posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
	else strcat( tag, "S" ); // S is for supine.
	if ( mass == MassSmall ) strcat( tag, "4" );
	else if ( mass == MassMedium ) strcat( tag, "6" );
	else if ( mass == MassLarge ) strcat( tag, "8" );
	else strcat( tag, "u" ); // for 'unspecified'
	tag[8] = 0;	// Make sure that the tag is no more than 8 characters long.

	// What is the target sequence? If not specified in the command line, use the default.
	if ( target_filename = ParseForTargetFile( params ) ) targetSequenceN = LoadSequence( targetSequence, target_filename );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) status = PrepTargeted( apparatus, params );
	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	else status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Verify that the subject is ready, in case they did something unexpected.
	if ( posture == PostureSeated ) {
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", MsgQueryReadySeated, OkToContinue );
	}
	else if ( posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", MsgQueryReadySupine, OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );

	// Indicate to the subject that we are ready to start and wait for their go signal.
	status = apparatus->WaitSubjectReady( "HandsOffCradle.bmp", MsgReadyToStart );
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

	// Wait until the subject gets to the target before moving on.
	apparatus->ShowStatus( MsgMoveToBlinkingTarget, "working.bmp" );
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( targetSequence[0] , targetedMovementOrientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget );
	else status = apparatus->WaitUntilAtHorizontalTarget( targetSequence[0], targetedMovementOrientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Collect some data while holding at the starting position.
	apparatus->Wait( baselineDuration );

	// Make sure that the target is turned back on if a timeout occured.
	// Normally this won't do anything.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) apparatus->VerticalTargetOn( targetSequence[0] );
	else apparatus->HorizontalTargetOn( targetSequence[0] );

	// Collect basline data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	apparatus->ShowStatus( "Move quickly to each lighted target. Don't start early. Stop at each target.", "working.bmp" );

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Step through the list of targets.
	apparatus->fComment( "Targets: %d", targetSequenceN );
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

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	SignalEndOfRecording( apparatus );
	status = apparatus->WaitSubjectReady( "PlaceMass.bmp", MsgTrialOver );
	if ( status == ABORT_EXIT ) exit( status );
	
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
	int n_post_hoc_steps = 2;
	int post_hoc_step = 0;
	
	// Was the manipulandum obscured?	
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occlusions exceed tolerance." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Check that we got a reasonable amount of movement.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for movement ..." );
	status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, targetedMovementDirection, "Movement amplitude out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// TODO: Are there more post hoc tests to be done here?

	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Post hoc tests completed." );

	// Indicate to the subject that they are done.
	// The first NULL parameter says to use the default picture.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	
	return( NORMAL_EXIT );
	
}



