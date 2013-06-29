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

//#define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.

/*********************************************************************************/


// Targeted trial parameters;
int delaySequence[] = { 0, 1, 0, 4, 0, 2, 0, 3, 0 };	// List of targets for point-to-point movements.
int delaySequenceN = sizeof( delaySequence ) / sizeof( *delaySequence );
int discreteTargets[2] = { 1, 9};

int discreteFalseStartTolerance = 5;
double discreteFalseStartThreshold = 1.0;
double discreteFalseStartHoldTime = 0.250;
double discreteFalseStartFilterConstant = 1.0;

double discreteMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double discreteMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double discreteCycleHysteresis = 10.0;	// Parameter used to adjust the detection of cycles. 

/*********************************************************************************/

int RunDiscrete( DexApparatus *apparatus, const char *params ) {

	Vector3 direction_vector;
	
	int status = 0;

	int bar_position = TargetBarRight;
	int direction = VERTICAL;
	bool eyes_closed = false;

	if ( params && strstr( params, "-hor" ) ) {
		bar_position = TargetBarLeft;
		direction = HORIZONTAL;
		apparatus->CopyVector( direction_vector, apparatus->kVector );
	}

	if ( params && strstr( params, "-ver" ) ) {
		bar_position = TargetBarRight;
		direction = VERTICAL;
		apparatus->CopyVector( direction_vector, apparatus->jVector );
	}

	if ( params && strstr( params, "-open" ) ) eyes_closed = false;
	if ( params && strstr( params, "-close" ) ) eyes_closed = true;



#ifndef SKIP_PREP

	// Check that the tracker is still aligned.
	status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[bar_position], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( PostureSeated, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	// I am calling this method separately, but it could be incorporated into SelectAndCheckConfiguration().
	apparatus->SetTargetPositions();
	
	// Send information about the actual configuration to the ground.
	// This is redundant, because the SelectAndCheckConfiguration() command will do this as well.
	// But I want to demonstrate that it can be used independent from the check as well.
	apparatus->SignalConfiguration();
	
	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Take a seat and attach the belts.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

#endif

	// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Pick up the manipulandum in the right hand.\nBe sure that thumb and forefinger are centered.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( 10.0, 0.25, 1.0 );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquiring data.
	apparatus->StartAcquisition( maxTrialDuration );

	// Wait until the subject gets to the target before moving on.
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( discreteTargets[0], uprightNullOrientation );
	else status = apparatus->WaitUntilAtHorizontalTarget( discreteTargets[0], uprightNullOrientation ); 
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
		
	if ( eyes_closed ) apparatus->WaitSubjectReady( "Close your eyes.\nPress OK when ready to continue." );
	else apparatus->WaitSubjectReady( "Close your eyes.\nPress OK when ready to continue." );

	if ( status == ABORT_EXIT ) exit( status );
	// Collect baseline data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Step through the list of targets.
	for ( int delay = 1; delay < delaySequenceN; delay++ ) {
		
		// Make a beep.
		apparatus->Beep();
		apparatus->MarkEvent( TRIGGER_MOVEMENT );	
		
		// Allow a random time before triggering the next target.
		// Takes into account the duration of the beep.
		apparatus->Wait( delaySequence[delay] - beepDuration );
		
	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Let the subject know that they are done.
	BlinkAll( apparatus );

	// Stop collecting data.
	ShowStatus( "Retrieving data ..." );
	apparatus->StopAcquisition();
	
	// Save the data.
	ShowStatus( "Saving data ..." );
	apparatus->SaveAcquisition( "TRGT" );
	
	// Check the quality of the data.
	ShowStatus( "Checking data ..." );


	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckMovementAmplitude( discreteMinMovementExtent, discreteMaxMovementExtent, direction_vector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement. 
	// We expect as many as there are items in the sequence. 
	// We accept if there are a few less.
	status = apparatus->CheckMovementCycles( delaySequenceN / 2 - 2, delaySequenceN, direction_vector, discreteCycleHysteresis, "Not as many movements as we expected.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	status = apparatus->CheckEarlyStarts( discreteFalseStartTolerance, discreteFalseStartHoldTime, 
		discreteFalseStartThreshold, discreteFalseStartFilterConstant, 
		"Too many early starts.\nPlease wait for the beep each time.\nWould you like to try again?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}



