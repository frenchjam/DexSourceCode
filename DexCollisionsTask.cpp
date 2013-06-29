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

//#define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/

// Collision trial parameters;
int collisionInitialTarget = 6;
int collisionUpTarget = 11;
int collisionDownTarget = 1;

#define UP		0
#define DOWN	1
int collisionSequenceN = 3;
int collisionSequence[] = { DOWN, UP, UP, DOWN, DOWN, DOWN, UP, DOWN, UP, UP };
double collisionTime = 2.0;
double collisionMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double collisionMovementThreshold = 10.0;

/*********************************************************************************/

int RunCollisions( DexApparatus *apparatus, const char *params ) {
	
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
		apparatus->Wait( initialMovementTime );
				
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
		apparatus->Wait( flashDuration );
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		apparatus->Wait( initialMovementTime - flashDuration );

		
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

	// Check if collision forces were within range.
	status = apparatus->CheckForcePeaks( 1.0, 5.0, 1 );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	// Same idea for acclerations.
	status = apparatus->CheckAccelerationPeaks( 1.0, 2.0, 1 );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}