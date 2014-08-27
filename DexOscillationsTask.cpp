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

	// Prompt the subject to put the target bar in the correct position.
	if ( bar_position == TargetBarRight ) {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "TappingFolded.bmp" : "TappingFolded.bmp" ), 
			PlaceTargetBarRightFolded, OkToContinue );
	}
	else {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "BarLeft.bmp" : "BarLeft.bmp" ), 
			"Place the target bar in the left position.%s", OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );


	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

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
	AddDirective( apparatus, InstructPickUpManipulandum, "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "OscillateV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "OscillateH.bmp";
	}

	if ( eyes == OPEN )	{
		AddDirective( apparatus, "You will hold the manipulandum upright and to the right of the blinking Target LED.", mtb );
		AddDirective( apparatus, "You will then oscillate between the lit targets, one full cycle per beep.", dsc );
		AddDirective( apparatus, "When the beeps will stop, you should continue the oscillations.", dsc );
	}
		// If we delete oscillation in discrete, then we don't need this condition here below any more.
	else {
		AddDirective( apparatus, "You will hold the manipulandum upright and to the right of the blinking Target LED.", mtb );
		AddDirective( apparatus, "You will then CLOSE your eyes and oscillate between the targets, one full cycle per beep.", dsc );
		AddDirective( apparatus, "When the beeps stop, you should continue the oscillations, keeping your eyes CLOSED.", dsc );
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
	static double frequency = 1.0;

	static DexMass mass = MassMedium;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename = 0;
	char *delay_filename = 0;

	fprintf( stderr, "     RunOscillations: %s\n", params );

	// How quickly should it oscillate?
	frequency = ParseForFrequency( apparatus, params );
	oscillationPeriod = 1.0 / frequency;
	oscillationDuration = ParseForDuration( apparatus, params );

	// Seated or supine?
	posture = ParseForPosture( params );

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Eyes open or closed?
	eyes = ParseForEyeState( params );

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

	double cop_tolerance = copTolerance;
	if ( ParseForNoCheck( params) ) cop_tolerance = 1000.0;

	// Construct the results filename tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else {
		strcpy( tag, "Grip" );
		strcat( tag, "O" ); // O for Oscillations.
		if ( posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
		else strcat( tag, "S" ); // S is for supine.
		if ( direction == VERTICAL ) strcat( tag, "V" );
		else strcat( tag, "H" );
		if ( mass == MassSmall ) strcat( tag, "4" );
		else if ( mass == MassMedium ) strcat( tag, "6" );
		else if ( mass == MassLarge ) strcat( tag, "8" );
		else strcat( tag, "u" ); // for 'unspecified'
	}

	// What are the limits of each oscillatory movement?
	if ( target_filename = ParseForRangeFile( params ) ) LoadTargetRange( oscillationTargets, target_filename );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) status = PrepOscillations( apparatus, params );
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

	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Start acquisition and acquire a baseline.
	// Presumably the manipulandum is not in the hand. 
	// It should have been left either in a cradle or the retainer at the end of the last action.
	apparatus->SignalEvent( "Initiating set of oscillation movements." );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Instruct subject to take the specified mass.
	//  and wait for confimation that he or she is ready.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	apparatus->ShowStatus( MsgCheckGripCentered, "working.bmp" );
	status = apparatus->WaitCenteredGrip( cop_tolerance, copForceThreshold, copWaitTime, MsgGripNotCentered, "alert.bmp" );
	if ( status == ABORT_EXIT ) exit( status );

	// Now wait until the subject gets to the target before moving on.
	apparatus->ShowStatus( MsgMoveToBlinkingTarget, "working.bmp" );
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( oscillationTargets[MIDDLE] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget, "MvToBlkVArrows.bmp"  );
	else status = apparatus->WaitUntilAtHorizontalTarget( oscillationTargets[MIDDLE], desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget, "MvToBlkHArrows.bmp"  ); 
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Double check that the subject has the specified mass.
	// If the correct mass is already on the manipulandum and out of the cradle, 
	//  this will move right on to the next step.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

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
	// We do the analysis only on the part of the recorded movement that occurs after
	//  the beeps stops in order to detect if the subject mistakenly stopped as well.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Now finish out the trial
	apparatus->ShowStatus( "Continue oscillating movements.\nMaintain the same rhythm.", "working.bmp" );
	apparatus->Wait( oscillationDuration - oscillationEntrainDuration );
	  
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final bit of data.
	apparatus->Wait( baselineDuration );

	// We're done.
	apparatus->TargetsOff();

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	SignalEndOfRecording( apparatus );
	status = apparatus->WaitSubjectReady( "PlaceMass.bmp", MsgTrialOver );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Take a couple of seconds of extra data with the manipulandum in the cradle so we get another zero measurement.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Stop acquiring.
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	apparatus->SignalEvent( "Acquisition completed." );
	apparatus->StopFilming();
	apparatus->StopAcquisition( "Error during saving." );

	if ( !ParseForNoCheck( params ) ) {

		// Check the quality of the data.
		// Note that for the oscillations protocol we perform the post-hoc test only on the
		//  part of the recorded movement after the beep stops (see above). This is to 
		//  detect whether the subject mistakenly stops moving when the beeps stop.
		//  This should be taken into account when defining the test criteria below.

		// Was the manipulandum obscured?
		apparatus->ShowStatus( "Checking visibility ...", "wait.bmp" );
		status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occluded too often. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that we got a reasonable amount of movement.
		apparatus->ShowStatus( "Checking for movement ...", "wait.bmp" );
		status = apparatus->CheckMovementAmplitude( oscillationMinMovementExtent, oscillationMaxMovementExtent, oscillationDirection, "Movement amplitude out of range. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that we got a reasonable number of oscillations.
		// Here I have set it to +/- 20% of the ideal number.
		apparatus->ShowStatus( "Checking number of Oscillations ...", "wait.bmp" );
		// oscillationMinCycles = 0.8 * ( oscillationDuration - oscillationEntrainDuration ) * frequency;
		// oscillationMaxCycles = 1.2 * ( oscillationDuration - oscillationEntrainDuration ) * frequency;
		// Widened range on oscillations so as not to constrain the subject.
		oscillationMinCycles = 3;
		oscillationMaxCycles = 50;
		status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, oscillationDirection, oscillationCycleHysteresis, "Number of oscillations out of range. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		apparatus->ShowStatus(  "Analysis completed.", "ok.bmp" );
	}

	return( NORMAL_EXIT );
	
}