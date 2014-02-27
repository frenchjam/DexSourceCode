/*********************************************************************************/
/*                                                                               */
/*                             DexOscillationsTask.cpp                           */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of point-to-point target movements to a sequence of targets.
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

#define UP		1
#define DOWN	-1
int collisionSequenceN = 6;
int collisionSequence[] = { DOWN, UP, UP, DOWN, DOWN, UP, UP, DOWN, UP, UP };
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

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	posture = ParseForPosture( params );
	direction = ParseForDirection( apparatus, params );
	if ( posture == PostureSeated ) {
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", "Seated?   Belts attached?   Wristbox on wrist?%s", OkToContinue );
	}
	else if ( posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", "Lying Down?  Belts attached?  Wristbox on wrist?%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );

	// Prompt the subject to deploy the tapping surfaces.
	status = apparatus->fWaitSubjectReady( "Unfolded.bmp", "Check that tapping surfaces are unfolded.%s", OkToContinue );
	if ( status == ABORT_EXIT ) exit( status );

	// Cancel any force offsets.
	RunTransducerOffsetCompensation( apparatus, params );

	// Instruct the subject on the task to be done.
	AddDirective( apparatus, "You will first pick up the manipulandum with\nthumb and index finger centered.", "InHand.bmp" );
	AddDirective( apparatus, "You should move to the center target\nwhenever it is blinking.", "MoveToBlinking.bmp" );
	AddDirective( apparatus, "You will then make collisions with the manipulandum.\nTap up or down according to the beeps and lights.", "Collision.bmp" );
	ShowDirectives( apparatus );

	return( NORMAL_EXIT );
}


/*********************************************************************************/

int RunCollisions( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	
	// Get the subject posture from the command line.
	DexSubjectPosture posture = ParseForPosture( params );

	// Collisions are always done with the mast on the right side.
	DexTargetBarConfiguration bar_position = TargetBarRight;

	// Collisions are done only in the vertical direction.
	int	direction = VERTICAL;
	Vector3 direction_vector = {0.0, 1.0, 0.0};
	Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename = 0;
	int tone, tgt;

	// What is the target sequence? If not specified in the command line, use the default.
	if ( target_filename = ParseForTargetFile( params ) ) collisionSequenceN = LoadSequence( target_filename, collisionSequence, MAX_SEQUENCE_ENTRIES );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepCollisions( apparatus, params );

	// Start acquisition and acquire a baseline.
	apparatus->SignalEvent( "Initiating set of collisions." );
	apparatus->StartAcquisition( "COLL", collisionMaxTrialTime );
	
	// Acquire
	Sleep( 1000 );

	// Which mass should be used for this set of trials?
	DexMass mass = ParseForMass( params );

	// Instruct subject to take the specified mass.
	// If the correct mass is already on the manipulandum and out of the cradle, 
	//  this will move right on to the next step.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

    // Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand.\n               OR               \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( collisionInitialTarget, desired_orientation );
	if ( status == ABORT_EXIT ) exit( ABORT_EXIT );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );
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

	// Was the manipulandum obscured?
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Check if trial was completed as instructed.
	apparatus->ShowStatus( "Checking movement directions ..." );
	status = apparatus->CheckMovementDirection( collisionWrongDirectionTolerance, direction_vector, collisionMovementThreshold );
	if ( status == ABORT_EXIT ) exit( ABORT_EXIT );

	// Check if collision forces were within range.
	apparatus->ShowStatus( "Checking collision forces ..." );
	status = apparatus->CheckForcePeaks( collisionMinForce, collisionMaxForce, collisionWrongForceTolerance );
	if ( status == ABORT_EXIT ) exit( ABORT_EXIT );

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	status = apparatus->SignalNormalCompletion( "ok.bmp", "Trial terminated successfully.\n\nPress <OK> to continue ..." );
	if ( status == ABORT_EXIT ) exit( status );

	return( NORMAL_EXIT );
	
}