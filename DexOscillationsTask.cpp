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
int oscillationUpperTarget = 2;						// Targets showing desired amplitude of cyclic movement.
int oscillationLowerTarget = 10;
int oscillationCenterTarget = 6;
double oscillationDuration = 30.0;
double oscillationEntrainDuration = 10.0; // Audio metronom duration on one trial
double osc_time_step = 1/1.5; // Oscillation Period
double oscillationMaxTrialTime = 30.0;		// Max time to perform the whole list of movements.
double oscillationMinMovementExtent = 19.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double oscillationMaxMovementExtent = 1000.0; //HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
int	oscillationMinCycles = 5;	// Minimum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
int oscillationMaxCycles = 40;	// Maximum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
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


	// Which mass should be used for this set of trials?
	DexMass mass = ParseForMass( params );

	direction = ParseForDirection( apparatus, params, posture, bar_position, direction_vector, desired_orientation );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	apparatus->ShowStatus( "Hardware configuration check ..." );
	status = apparatus->SelectAndCheckConfiguration( posture, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Pictures\\Folded.bmp", "Check that tapping surfaces are folded.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

		// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

		// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Pictures\\Manip_in_hand.bmp", "Hold the manipulandum with thumb and \nforefinger centered. \nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );
   
	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n Or \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );


	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( "Pictures\\Oscillation.bmp", "Align the manipulandum with the flashing target \nand then oscillate it between the two lid targets \nfollowing the frequency given by sound. " );
	if ( status == ABORT_EXIT ) exit( status );

	// Light up the central target.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationCenterTarget );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( oscillationCenterTarget, desired_orientation );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Start acquiring data.
	apparatus->StartAcquisition( "OSCI", oscillationMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationLowerTarget );
	apparatus->TargetOn( oscillationUpperTarget );
	
	// Output a sound pattern to establish the oscillation frequency.
	// This will play for the first 10 seconds of the trial.
	
	double time;
	for ( time = 0.0; time < oscillationEntrainDuration; time += osc_time_step ) {
		int tone = 3.9; //floor( 3.9 * ( sin( time * 2.0 * PI ) + 1.0 ) );
		apparatus->SetSoundStateInternal( tone, 1 );
		apparatus->Wait( osc_time_step/2 - 0.01 );
		apparatus->SetSoundStateInternal( tone, 0 );
        apparatus->Wait( osc_time_step/2 - 0.01 );
	}
	apparatus->SetSoundState( 4, 0 );

	// Now finish out the trial
	apparatus->Wait( oscillationDuration - oscillationEntrainDuration );
	  
	// Blink the targets to signal the end of the recording.
	BlinkAll(apparatus);

	// Stop acquiring.
	apparatus->ShowStatus( "Retrieving data ..." );
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ..." );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum occluded too often." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementAmplitude( oscillationMinMovementExtent, oscillationMaxMovementExtent, oscillationDirection, "Movement extent out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, oscillationDirection, oscillationCycleHysteresis, "Number of oscillations out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
		
	return( NORMAL_EXIT );
	
}