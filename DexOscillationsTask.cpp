/*********************************************************************************/
/*                                                                               */
/*                             DexOscillationsTask.cpp                           */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of oscillatory movements.
 */

/*********************************************************************************/

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

// Oscillation trial parameters.
int oscillationUpperTarget = 9;						// Targets showing desired amplitude of cyclic movement.
int oscillationLowerTarget = 3;
int oscillationCenterTarget = 6;
double oscillationTime = 10.0;
double oscillationMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double oscillationMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double oscillationMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
int	oscillationMinCycles = 6;	// Minimum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
int oscillationMaxCycles = 20;	// Maximum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
double oscillationCycleHysteresis = 10.0;	// Parameter used to adjust the detection of cycles. 
Vector3	oscillationDirection = {0.0, 1.0, 0.0};	// Oscillations are nominally in the vertical direction. Could change at some point, I suppose.

/*********************************************************************************/

int RunOscillations( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;
	
	static int	direction = VERTICAL;
	static int bar_position = TargetBarRight;
	static int posture = PostureSeated;
	static Vector3 direction_vector = {0.0, 1.0, 0.0};
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	direction = ParseForDirection( apparatus, params, posture, bar_position, direction_vector, desired_orientation );

	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[posture], TargetBarString[bar_position], TappingSurfaceString[TappingUnknown] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, TappingUnknown );
	if ( status == ABORT_EXIT ) exit( status );

	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( "Move to the flashing target.\nWhen 2 new targets appear, make oscillating\nmovements at approx. 1 Hz. " );
	if ( status == ABORT_EXIT ) exit( status );

	// Light up the central target.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationCenterTarget );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( oscillationCenterTarget, desired_orientation );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Start acquiring data.
	apparatus->StartAcquisition( oscillationMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationLowerTarget );
	apparatus->TargetOn( oscillationUpperTarget );
	
	// Measure data during oscillations performed over a fixed duration.
	apparatus->Wait( oscillationTime );
	
	// Stop acquiring.
	ShowStatus( "Retrieving data ..." );
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	ShowStatus( "Saving data ..." );
	apparatus->SaveAcquisition( "OSCI" );
	
	// Check the quality of the data.
	ShowStatus( "Checking data ..." );
	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementAmplitude( oscillationMinMovementExtent, oscillationMaxMovementExtent, oscillationDirection, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, oscillationDirection, oscillationCycleHysteresis, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
		
	return( NORMAL_EXIT );
	
}