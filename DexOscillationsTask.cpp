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
#define UPPER 2
#define MIDDLE 1
#define LOWER 0
int oscillationTargets[3] = { 2, 6, 10};		// Targets showing desired amplitude of cyclic movement.

double oscillationDuration = 30.0;				// Total duration of a single trial
double oscillationEntrainDuration = 10.0;		// Audio metronom duration on one trial
double oscillationPeriod = 1.0 / 1.5;			// Oscillation period (per full cycle );

double oscillationMinMovementExtent = 19.0;		// Minimum amplitude along the movement direction (nominally Y). Set to 1000.0 to simulate error.
double oscillationMaxMovementExtent = 1000.0;	// Maximum amplitude along the movement direction. Set to 1.0 to simulate error.

int	oscillationMinCycles = 5;					// Minimum cycles along the movement direction. Set to 1000.0 to simulate error.
int oscillationMaxCycles = 40;					// Maximum cycles along the movement direction. Set to 1000.0 to simulate error.
double oscillationCycleHysteresis = 10.0;		// Parameter used to adjust the detection of cycles. 

Vector3	oscillationDirection = {0.0, 1.0, 0.0};	// Oscillations are nominally in the vertical direction. Could change at some point, I suppose.

/*********************************************************************************/

int PrepOscillations( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	char *target_filename = 0;
	char *mtb, *dsc;
	
	int	direction = ParseForDirection( apparatus, params );
	int eyes = ParseForEyeState( params );
	DexSubjectPosture posture = ParseForPosture( params );
	DexTargetBarConfiguration bar_position;

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

	// Prompt the subject to stow the tapping surfaces. Only necessary of doing vertical movement with the target bar on the right.
	if ( bar_position == TargetBarRight )  {
		status = apparatus->fWaitSubjectReady( "Folded.bmp", "Check that tapping surfaces are folded.%s", OkToContinue );
		if ( status == ABORT_EXIT ) exit( status );
	}

	// Cancel any force offsets.
	RunTransducerOffsetCompensation( apparatus, params );

	// Instruct the subject on the task to be done.
	
	// Show them the targets that will be used.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( oscillationTargets[UPPER] );
		apparatus->VerticalTargetOn( oscillationTargets[LOWER] );
	}
	else {
		apparatus->HorizontalTargetOn( oscillationTargets[UPPER] );
		apparatus->HorizontalTargetOn( oscillationTargets[LOWER] );
	}

	// Describe how to do the task, according to the desired conditions.
	GiveDirective( apparatus, "You will first pick up the manipulandum with\nthumb and index finger centered.", "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "OscillateV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "OscillateH.bmp";
	}

	if ( eyes == OPEN )	{
		GiveDirective( apparatus, "To start, move to the target that is blinking.", mtb );
		GiveDirective( apparatus, "Oscillate continuously between the two lit targets\nfollowing the frequency given by the sound.\nKeep your eyes OPEN the entire time.", dsc );
	}
	else {
		GiveDirective( apparatus, "To start, move to the target that is blinking.\nThen CLOSE your eyes.", mtb );
		GiveDirective( apparatus, "On each beep,move quickly and accurately\nto the remebered location of the other target.", dsc );
	}

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunOscillations( DexApparatus *apparatus, const char *params ) {
	
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

	double frequency = ParseForFrequency( apparatus, params );
	oscillationPeriod = 1.0 / frequency;
	oscillationDuration = ParseForDuration( apparatus, params );


	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) {
		bar_position = TargetBarRight;
		apparatus->CopyVector( oscillationDirection, apparatus->jVector );
	}
	else {
		bar_position = TargetBarLeft;
		apparatus->CopyVector( oscillationDirection, apparatus->kVector );
	}

	// Eyes open or closed?
	eyes = ParseForEyeState( params );

	// What are the three targets to be used (lower, middle, upper) If not specified in the command line, use the default.
	if ( target_filename = ParseForTargetFile( params ) ) LoadSequence( target_filename, oscillationTargets, 3 );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status == ABORT_EXIT ) return( status );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) PrepOscillations( apparatus, params );

	// Start acquisition and acquire a baseline.
	apparatus->SignalEvent( "Initiating set of oscillation movements." );
	apparatus->StartAcquisition( "OSCI", maxTrialDuration );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n      Or      \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	// Now wait until the subject gets to the target before moving on.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( oscillationTargets[MIDDLE], desired_orientation );
	else status = apparatus->WaitUntilAtHorizontalTarget( oscillationTargets[MIDDLE], desired_orientation );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( oscillationTargets[LOWER] );
		apparatus->VerticalTargetOn( oscillationTargets[UPPER] );
	}
	else {
		apparatus->HorizontalTargetOn( oscillationTargets[LOWER] );
		apparatus->HorizontalTargetOn( oscillationTargets[UPPER] );
	}
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Output a sound pattern to establish the oscillation frequency.
	// This will play for the first part of each trial trial.
	
	double time;
	for ( time = 0.0; time < oscillationEntrainDuration; time += oscillationPeriod ) {
		int tone = 3.9; //floor( 3.9 * ( sin( time * 2.0 * PI ) + 1.0 ) );
		apparatus->SetSoundState( tone, 1 );
		apparatus->Wait( oscillationPeriod / 2.0 - 0.01 );
		apparatus->SetSoundState( tone, 0 );
        apparatus->Wait( oscillationPeriod / 2.0 - 0.01 );
	}
	apparatus->MarkEvent( END_ENTRAIN );
	apparatus->SetSoundState( 4, 0 );

	// Now finish out the trial
	apparatus->Wait( oscillationDuration - oscillationEntrainDuration );
	  
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );
	apparatus->Wait( 1.0 );

	// Blink the targets to signal the end of the recording.
	BlinkAll(apparatus);

	// Stop acquiring.
	apparatus->ShowStatus( "Retrieving data ..." );
	apparatus->SignalEvent( "Terminating recording." );
	apparatus->StopAcquisition();
	
	// Check the quality of the data.
	apparatus->ShowStatus( "Checking data ..." );
	
	// Was the manipulandum obscured?
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum occluded too often." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementAmplitude( oscillationMinMovementExtent, oscillationMaxMovementExtent, oscillationDirection, "Movement extent out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	// Here I have set it to +/- 20% of the ideal number.
	oscillationMinCycles = 0.8 * oscillationDuration * frequency;
	oscillationMaxCycles = 1.2 * oscillationDuration * frequency;
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, oscillationDirection, oscillationCycleHysteresis, "Number of oscillations out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
		
	return( NORMAL_EXIT );
	
}