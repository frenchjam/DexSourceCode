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

#define UP		0
#define DOWN	1
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

int RunCollisions( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;
	
	static int	direction = VERTICAL;
	static int bar_position = TargetBarRight;
	static int posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	// Which mass should be used for this set of trials?
	DexMass mass = ParseForMass( params );

	direction = ParseForDirection( apparatus, params, posture, bar_position, direction_vector, desired_orientation );

		// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	apparatus->ShowStatus( "Hardware configuration check ..." );
	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Unfolded.bmp", "Check that tapping surfaces are unfolded.\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Belts.bmp", "Seated?   Belts attached?   Wristbox on wrist?\n\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->SignalEvent( "Initiating set of collisions." );
	Sleep( 1000 );
	apparatus->StartAcquisition( "COLL", collisionMaxTrialTime );
	
	// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "InHand.bmp", "Hold the manipulandum with thumb and \nindexfinger centered. \nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );
   
    // Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n Or \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );
	
    // Instruct subject to perform collision with the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Collision.bmp", "Make collision with the manipulandum and the \ntapping surface following beep and lid target. \n\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( collisionInitialTarget, desired_orientation );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );
	for ( int target = 0; target < collisionSequenceN; target++ ) {
		
		// Ready to start, so light up starting point target.
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		
		// Allow a fixed time to reach the starting point before triggering the first movement.
		//apparatus->Wait( initialMovementTime );
				
		apparatus->TargetsOff();
		if ( collisionSequence[target] ) {
		int tone = 2.0;
		apparatus->SetSoundStateInternal( tone, 1 );
			apparatus->VerticalTargetOn( collisionUpTarget );
			apparatus->VerticalTargetOn( collisionUpTarget-1 );
			apparatus->VerticalTargetOn( collisionUpTarget-2 );
			apparatus->VerticalTargetOn( collisionUpTarget-3 );
			apparatus->VerticalTargetOn( collisionUpTarget-4 );
			apparatus->VerticalTargetOn( collisionUpTarget-5 );
			apparatus->MarkEvent( TRIGGER_MOVE_UP);
		}
		else {
			int tone = 3.9;
		    apparatus->SetSoundStateInternal( tone, 1 );
			apparatus->VerticalTargetOn( collisionDownTarget );
			apparatus->VerticalTargetOn( collisionDownTarget+1 );
			apparatus->VerticalTargetOn( collisionDownTarget+2 );
			apparatus->VerticalTargetOn( collisionDownTarget+3 );
			apparatus->VerticalTargetOn( collisionDownTarget+4 );
            apparatus->VerticalTargetOn( collisionDownTarget+5 );
			apparatus->MarkEvent( TRIGGER_MOVE_DOWN );
		}

		// Turn off the LED (sound) after a brief instant and turn the
		apparatus->Wait( 0.5 );
		apparatus->SetSoundState( 4, 0 );
        apparatus->Wait( 0.25 );
		apparatus->TargetsOff();
	
		// Wait a fixed time to finish the movement.
		apparatus->Wait( 1.5 - 0.74 );
		// Light the center target to bring the hand back to the starting point.
		//apparatus->TargetOn( collisionInitialTarget );

	
	}
	
	apparatus->TargetsOff();

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	apparatus->SignalEvent( "Acquisition terminated." );
	
	// Check if trial was completed as instructed.
	apparatus->ShowStatus( "Checking movement directions ..." );
	status = apparatus->CheckMovementDirection( collisionWrongDirectionTolerance, direction_vector, collisionMovementThreshold );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Check if collision forces were within range.
	apparatus->ShowStatus( "Checking collision forces ..." );
	status = apparatus->CheckForcePeaks( collisionMinForce, collisionMaxForce, collisionWrongForceTolerance );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}