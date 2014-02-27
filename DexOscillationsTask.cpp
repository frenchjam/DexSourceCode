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
int oscillationTargets[3] = { 0, 4, 8};		// Targets showing desired amplitude of cyclic movement.

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
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", "Seated?\nBelts attached?\nWristbox on wrist?%s", OkToContinue );
	}
	else if ( posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", "Lying Down?  Belts attached?/nWristbox on wrist?%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );

	// Prompt the subject to stow the tapping surfaces. Only necessary of doing vertical movement with the target bar on the right.
	if ( bar_position == TargetBarRight )  {
		status = apparatus->fWaitSubjectReady( "Folded.bmp", "Check that tapping surfaces are folded.%s", OkToContinue );
		if ( status == ABORT_EXIT ) exit( status );
	}

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
	AddDirective( apparatus, "You will pick up the manipulandum\nwith thumb and index finger centered.", "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "OscillateV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "OscillateH.bmp";
	}

	if ( eyes == OPEN )	{
		AddDirective( apparatus, "To start you will move\nto the blinking target.", mtb );
		AddDirective( apparatus, "You will then oscillate between targets, one full cycle per beep.", dsc );
		AddDirective( apparatus, "When the beeps stop, continue oscillating. Keep your eyes OPEN.", dsc );
	}
	else {
		AddDirective( apparatus, "To start you will move\nto the blinking target.", mtb );
		AddDirective( apparatus, "You will then CLOSE your eyes and move between targets, one full cycle per beep.", dsc );
		AddDirective( apparatus, "When the beeps stop, continue oscillating. Keep your eyes CLOSED.", dsc );
	}

	ShowDirectives( apparatus );

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

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	status = apparatus->WaitSubjectReady( "cradles.bmp", "We are ready to start.\nIs the manipulandum in a cradle?\nRemove hand and press <OK> to start." );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquisition and acquire a baseline.
	// Presumably the manipulandum is not in the hand. 
	// It should have been left either in a cradle or the retainer at the end of the last action.
	apparatus->SignalEvent( "Initiating set of oscillation movements." );
	apparatus->StartFilming();
	apparatus->StartAcquisition( "OSCI", maxTrialDuration );
	apparatus->ShowStatus( "Acquiring baseline. Please wait ...", "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	apparatus->ShowStatus( "Make sure that the grip is centered.", "working.bmp" );
	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n      Or      \n Fingers not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	// Now wait until the subject gets to the target before moving on.
	apparatus->ShowStatus( "Trial started.\nMove to blinking target.", "working.bmp" );
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( oscillationTargets[MIDDLE], desired_orientation );
	else status = apparatus->WaitUntilAtHorizontalTarget( oscillationTargets[MIDDLE], desired_orientation );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	// Indicate what to do next.
	apparatus->ShowStatus( "Start oscillating movements.\nOne cycle per beep.", "working.bmp" );

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

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Now finish out the trial
	apparatus->ShowStatus( "Continue oscillating movements.\nMaintain the same rhythm.", "working.bmp" );
	apparatus->Wait( oscillationDuration - oscillationEntrainDuration );
	  
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );
	apparatus->Wait( 1.0 );

	// We're done.
	apparatus->TargetsOff();

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	SignalEndOfRecording( apparatus );
	status = apparatus->WaitSubjectReady( "cradles.bmp", "Trial terminated.\nPlease place the maniplandum in the empty cradle." );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->ShowStatus( "Acquiring baseline. Please wait ...", "wait.bmp" );
	apparatus->Wait( 1.0 );

	// Stop acquiring.
	apparatus->StopAcquisition();
	apparatus->StopFilming();
	apparatus->SignalEvent( "Acquisition terminated." );
	apparatus->HideStatus();

	// Check the quality of the data.
	int n_post_hoc_steps = 3;
	int post_hoc_step = 0;

	// Was the manipulandum obscured?
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum occluded too often." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking for movement ..." );
	status = apparatus->CheckMovementAmplitude( oscillationMinMovementExtent, oscillationMaxMovementExtent, oscillationDirection, "Movement extent out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable number of oscillations.
	// Here I have set it to +/- 20% of the ideal number.
	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Checking number of oscillations ..." );
	oscillationMinCycles = 0.8 * ( oscillationDuration - oscillationEntrainDuration ) * frequency;
	oscillationMaxCycles = 1.2 * ( oscillationDuration - oscillationEntrainDuration ) * frequency;
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, oscillationDirection, oscillationCycleHysteresis, "Number of oscillations out of range." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	AnalysisProgress( apparatus, post_hoc_step++, n_post_hoc_steps, "Post hoc tests completed." );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
		
	return( NORMAL_EXIT );
	
}