/*********************************************************************************/
/*                                                                               */
/*                             DexCollisionsTask.cpp                           */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of collisions to the unfolded pad.
 */

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

// Collision trial parameters;
int collisionInitialTarget = 6;
int collisionUpTarget = 11;
int collisionDownTarget = 1;

int collisionSequenceN = 20;
int collisionSequence[MAX_SEQUENCE_ENTRIES] = { DOWN, UP, DOWN, DOWN, UP, DOWN, UP, DOWN, UP, UP,DOWN, UP, UP, DOWN, DOWN, UP, UP, DOWN, UP, DOWN };
double collisionTime = 1.5;
double collisionMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double collisionMovementThreshold = 10.0;

int collisionWrongDirectionTolerance = 5;
double collisionMinForce = 1.0;
double collisionMaxForce = 5.0;
int collisionWrongForceTolerance = 5;

/*********************************************************************************/

int PrepCollisions( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	
	int	direction = VERTICAL;
	DexTargetBarConfiguration bar_position = TargetBarRight;
	DexSubjectPosture posture = PostureSeated;
	Vector3 direction_vector = {0.0, 1.0, 0.0};
	Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};
	char *target_filename = 0;

	// Prompt the subject to put the target bar in the correct position.
	status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "Unfolded.bmp" : "Unfolded.bmp" ), 
		"Place the target bar in the right position and open the tapping surfaces.%s", OkToContinue );
	if ( status == ABORT_EXIT ) exit( status );


	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	posture = ParseForPosture( params );
	direction = ParseForDirection( apparatus, params );
//	if ( posture == PostureSeated ) {
//		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", MsgQueryReadySeated, OkToContinue );
//	}
//	else if ( posture == PostureSupine ) {
//		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", MsgQueryReadySupine, OkToContinue );
//	}
//	if ( status == ABORT_EXIT ) exit( status );


	// Instruct the subject on the task to be done.
	AddDirective( apparatus, InstructPickUpManipulandum, "InHand.bmp" );
	AddDirective( apparatus, "You will move the manipulandum to the blinking target.", "MvToBlkV.bmp" );
	AddDirective( apparatus, "You will then tap up or down according to the beeps and targets and return to the center.", "Collision.bmp" );
	ShowDirectives( apparatus );

	return( NORMAL_EXIT );
}


/*********************************************************************************/

int RunCollisions( DexApparatus *apparatus, const char *params ) {

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
	char tag[5] = "Co";			// Co is for Collisions.

	int tone, tgt;

	fprintf( stderr, "     RunCollisions: %s\n", params );

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Seated or supine?
	posture = ParseForPosture( params );
	if ( posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
	else strcat( tag, "S" ); // S is for supine.

	// What is the target sequence? If not specified in the command line, use the default.
	// Here we expect a sequence of +1 or -1 values, corresponding to an upward or downward tap.
	// Note that we use the same mechanism as that used to define the target sequence for targeted,
	//  movements, i.e. one can use a qualifier :# to specify which sequence in a file with multiple sequences.
	if ( target_filename = ParseForTargetFile( params ) ) collisionSequenceN = LoadSequence( collisionSequence, target_filename );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	else {
		status = CheckInstall( apparatus, posture, bar_position );
		if ( status != NORMAL_EXIT ) return( status );
	}

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepCollisions( apparatus, params );

	// Indicate to the subject that we are ready to start and wait for their go signal.
	status = apparatus->WaitSubjectReady( "OffsetMeasurement.bmp", MsgReadyToStart );
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
	apparatus->WaitUntilAtVerticalTarget( collisionInitialTarget, desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget );
	if ( status == ABORT_EXIT ) exit( status );

	// Collect some data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	apparatus->ShowStatus( "Tap upward or downward according to the beep and the lighted targets.", "working.bmp" );

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Wait a little to give the subject time to react, in case they were looking at the screen.
	apparatus->Wait( baselineDuration );

	for ( int target = 0; target < collisionSequenceN; target++ ) {
		
		// Ready to start, so light up starting point target.
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
						
		apparatus->TargetsOff();
		if ( collisionSequence[target] == UP ) {
			tone = 2;
			apparatus->SetSoundStateInternal( tone, 1 );
			for ( tgt = collisionInitialTarget; tgt <= collisionUpTarget; tgt++ ) apparatus->VerticalTargetOn( tgt );
			apparatus->MarkEvent( TRIGGER_MOVE_UP );
		}
		else if ( collisionSequence[target] == DOWN ) {
			tone = 3; // Why was this 3.9 and not 3?
			apparatus->SetSoundStateInternal( tone, 1 );
			for ( tgt = collisionInitialTarget; tgt >= collisionDownTarget; tgt--) apparatus->VerticalTargetOn( tgt );
			apparatus->MarkEvent( TRIGGER_MOVE_DOWN );
		}
		// If the direction is neither UP nor DOWN, go to the center.
		// This option may never be used, but who knows.
		else {
			tone = 1;
			apparatus->SetSoundStateInternal( tone, 1 );
			apparatus->TargetOn( collisionInitialTarget );
		}

		// Turn off the sound after a brief instant.
		apparatus->Wait( 0.5 );
		apparatus->SetSoundState( 4, 0 );
		// Then turn off the targets after the sound finishes.
        apparatus->Wait( 0.25 );
		apparatus->TargetsOff();
		apparatus->TargetOn(collisionInitialTarget);
	
		// Wait a fixed time to finish the movement.
		// The 0.74 compensates for the two Wait() calls above.
		apparatus->Wait( collisionTime - 0.74 );

		// Light the center target to bring the hand back to the starting point.
		//apparatus->TargetOn( collisionInitialTarget );

	
	}
	
	// We're done.
	apparatus->TargetsOff();

	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	SignalEndOfRecording( apparatus );
	status = apparatus->WaitSubjectReady( "PlaceMass.bmp", MsgTrialOver );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->Wait( 1.0 );

	// Stop acquiring.
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	apparatus->SignalEvent( "Acquisition terminated." );
	apparatus->StopFilming();
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ...", "working.bmp" );
	int n_post_hoc_steps = 4;
	int post_hoc_step = 0;

	// Was the manipulandum obscured?
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum occluded too often." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Check if trial was completed as instructed.
	apparatus->ShowStatus( "Checking movement directions ..." );
	status = apparatus->CheckMovementDirection( collisionWrongDirectionTolerance, direction_vector, collisionMovementThreshold, "Too many starts in the wrond direction." );
	if ( status == ABORT_EXIT ) exit( ABORT_EXIT );

	// Check if collision forces were within range.
	apparatus->ShowStatus( "Checking collision forces ..." );
	status = apparatus->CheckForcePeaks( collisionMinForce, collisionMaxForce, collisionWrongForceTolerance, "Collision forces outside acceptable range." );
	if ( status == ABORT_EXIT ) exit( ABORT_EXIT );

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	status = apparatus->SignalNormalCompletion( "ok.bmp", "Trial terminated successfully.\n\nPress <OK> to continue ..." );
	if ( status == ABORT_EXIT ) exit( status );

	return( NORMAL_EXIT );
	
}