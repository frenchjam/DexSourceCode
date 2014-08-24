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

// Targeted trial parameters;
double	delaySequence[1024] = { 1, 1.5, 1, 2, 1.5, 3, 1, 2, 1 };	// Delays between the discrete movements.
int		delaySequenceN = 9;
int		discreteTargets[3] = {3, 5, 7};

int discreteFalseStartTolerance = 5;
double discreteFalseStartThreshold = 0.50;
double discreteFalseStartHoldTime = 0.010;
double discreteFalseStartFilterConstant = 1.0;

double discreteMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double discreteMaxMovementExtent = 200.0;	// Maximum amplitude along the movement direction (Y). Set to 1g.0 to simulate error.
double discreteCycleHysteresis = 10.0;	// Parameter used to adjust the detection of cycles. 
Vector3	discreteMovementDirection = {0.0, 1.0, 0.0};	// Movements are nominally in the vertical direction. Could change at some point, I suppose.

/*********************************************************************************/

int PrepDiscrete( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	char *target_filename = 0;
	char *mtb, *dsc;
	
	// What conditions are we about to use.
	DexTargetBarConfiguration bar_position;
	DexSubjectPosture posture = ParseForPosture( params );
	int eyes = ParseForEyeState( params );
	int	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

	// Prompt the subject to put the target bar in the correct position.
	if ( bar_position == TargetBarRight ) {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "TappingFolded.bmp" : "TappingFolded.bmp" ), 
			PlaceTargetBarRightFolded, OkToContinue );
	}
	else {
		status = apparatus->fWaitSubjectReady( ( posture == PostureSeated ? "BarLeft.bmp" : "BarLeft.bmp" ), 
			PlaceTargetBarLeft, OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );


	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Show them the targets that will be used.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( discreteTargets[LOWER] );
		apparatus->VerticalTargetOn( discreteTargets[UPPER] );
	}
	else {
		apparatus->HorizontalTargetOn( discreteTargets[LOWER] );
		apparatus->HorizontalTargetOn( discreteTargets[UPPER] );
	}

	// Describe how to do the task, according to the desired conditions.
	AddDirective( apparatus, InstructPickUpManipulandum, "InHand.bmp" );
	if ( direction == VERTICAL ) {
		mtb = "MvToBlkV.bmp";
		dsc = "DiscreteV.bmp";
	}
	else {
		mtb = "MvToBlkH.bmp";
		dsc = "DiscreteH.bmp";
	}

	AddDirective( apparatus, "To start, you will move the manipulandum to the blinking Target LED.", mtb );
	// It would be good to have here pictures of the manipulandum properly aligned to the target.
	// I shorted the names wrt official OpNoms to make the messages fit with the word 'upright'.
	AddDirective( apparatus, "On each beep, you will move quickly and accurately to the other lit Target LED.", dsc );
	AddDirective( apparatus, "Before each trial, you will be instructed to keep your eyes OPEN or CLOSED.", "EyesOpenClosed.bmp" );
	// According to the astronaut representative, it would be better if the picture changed at each new instruction.
	// It would be good to have a picture about eyes being open or closed.
	AddDirective( apparatus, "Remember to wait for each beep and come to a full stop at each Target LED.", "info.bmp" );
	if ( direction == VERTICAL ) AddDirective( apparatus, "Place manipulandum upright and to the right of Mast and align center with each target.", mtb );
	else AddDirective( apparatus, "Place manipulandum upright and to the right of Tablet and align center with each target.", mtb );
	ShowDirectives( apparatus );

	return( NORMAL_EXIT );
}


/*********************************************************************************/

int RunDiscrete( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;

	// These are static so that if the params string does not specify a value,
	//  whatever was used the previous call will be used again.
	static int	direction = VERTICAL;
	static int	eyes = OPEN;
	static DexMass mass = MassMedium;
	static DexTargetBarConfiguration bar_position = TargetBarRight;
	static DexSubjectPosture posture = PostureSeated;
	static Quaternion desired_orientation = {0.0, 0.0, 0.0, 1.0};

	char *target_filename = 0;
	char *delay_filename = 0;

	fprintf( stderr, "     RunDiscrete: %s\n", params );

	double cop_tolerance = copTolerance;
	if ( ParseForNoCheck( params) ) cop_tolerance = 1000.0;

	// Which mass should be used for this set of trials?
	mass = ParseForMass( params );

	// Seated or supine?
	posture = ParseForPosture( params );

	// Horizontal or vertical movements?
	direction = ParseForDirection( apparatus, params );
	if ( direction == VERTICAL ) {
		bar_position = TargetBarRight;
		apparatus->CopyVector( discreteMovementDirection, apparatus->jVector );
	}
	else {
		bar_position = TargetBarLeft;
		apparatus->CopyVector( discreteMovementDirection, apparatus->kVector );
	}

	// Eyes open or closed?
	eyes = ParseForEyeState( params );

	// Construct the results filename tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else {
		strcpy( tag, "Grip" );
		strcat( tag, "D" ); // C for Discrete.
		if ( posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
		else strcat( tag, "S" ); // S is for supine.
		if ( direction == VERTICAL ) strcat( tag, "V" );
		else strcat( tag, "H" );
		if ( eyes == OPEN ) strcat( tag, "o" ); // o for open
		else strcat( tag, "c" ); // c for closed
	}

	// What is the sequence of delays? If not specified in the command line, use the default.
	if ( delay_filename = ParseForDelayFile( params ) ) delaySequenceN = LoadSequence( delaySequence, delay_filename );

	// What are the limits of each discrete movement?
	if ( target_filename = ParseForRangeFile( params ) ) LoadTargetRange( discreteTargets, target_filename );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) status = PrepDiscrete( apparatus, params );
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

	status = apparatus->WaitSubjectReady( "HandsOffCradle.bmp", MsgReadyToStart );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Start acquisition and acquire a baseline.
	// Presumably the manipulandum is not in the hand. 
	// It should have been left either in a cradle or the retainer at the end of the last action.
	apparatus->SignalEvent( "Initiating set of discrete movements." );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );
	
	// Instruct subject to take the specified mass.
	// If the correct mass is already on the manipulandum and out of the cradle, 
	//  this will move right on to the next step.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Indicate to the subject that we are ready to start and wait for their go signal.
	if ( eyes == OPEN ) status = apparatus->WaitSubjectReady( "Eyesopen.bmp", "You will perform the next set of discrete movements with eyes OPEN." );
	else status = apparatus->WaitSubjectReady( "EyesClosed.bmp", "You will perform the next set of discrete movements with eyes CLOSED. (But don't close them yet!)." );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	apparatus->ShowStatus( MsgCheckGripCentered, "working.bmp" );
	status = apparatus->WaitCenteredGrip( cop_tolerance, copForceThreshold, copWaitTime, MsgGripNotCentered, "alert.bmp" );
	if ( status == ABORT_EXIT ) exit( status );

	// Wait until the subject gets to the target before moving on.
	apparatus->ShowStatus( MsgMoveToBlinkingTarget, "working.bmp" );
	apparatus->TargetsOff();
	// Light up the pair of targets.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( discreteTargets[LOWER] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget );
	else status = apparatus->WaitUntilAtHorizontalTarget( discreteTargets[LOWER] , desired_orientation, defaultPositionTolerance, defaultOrientationTolerance, waitHoldPeriod, waitTimeLimit, MsgTooLongToReachTarget ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Double check that the subject has the specified mass.
	// If the correct mass is already on the manipulandum and out of the cradle, 
	//  this will move right on to the next step.
	status = apparatus->SelectAndCheckMass( mass );
	if ( status == ABORT_EXIT ) exit( status );

	// Collect some data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	// Need to light the pair of targets again.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) {
		apparatus->VerticalTargetOn( discreteTargets[LOWER] );
		apparatus->VerticalTargetOn( discreteTargets[UPPER] );
	}
	else {
		apparatus->HorizontalTargetOn( discreteTargets[LOWER] );
		apparatus->HorizontalTargetOn( discreteTargets[UPPER] );
	}
	
	// Indicate what to do next.
	if ( eyes == OPEN ) apparatus->ShowStatus( "Start point-to-point movements with eyes OPEN.", "Eyesopen.bmp" ); 
	else apparatus->ShowStatus( "CLOSE your eyes and start point-to-point movements.", "EyesClosed.bmp" ); 

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Give some time to read the message.
	apparatus->Wait( 2.0 );
		
	// Wait a little to give the subject time to react, in case they were looking at the screen.
	apparatus->Wait( baselineDuration );

	// Step through the list of delays.
	for ( int delay = 1; delay < delaySequenceN; delay++ ) {
		
		// Make a beep.
		apparatus->MarkEvent( TRIGGER_MOVEMENT );	
		apparatus->Beep();
		
		// Allow a random time before triggering the next target.
		// Takes into account the duration of the beep.
		apparatus->Wait( delaySequence[delay] - beepDuration );
		
	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final bit of data.
	apparatus->Wait( baselineDuration );
	
	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	apparatus->Beep();
	apparatus->Beep();
	apparatus->Beep();
	
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
	apparatus->StopAcquisition( "Error during file save." );
	
	if ( !ParseForNoCheck( params ) ) {
		// Check the quality of the data.
		apparatus->ShowStatus( "Checking data ...", "wait.bmp" );
		int n_post_hoc_steps = 4;
		int post_hoc_step = 0;

		// Was the manipulandum obscured?
		apparatus->ShowStatus(  "Checking visibility ...", "wait.bmp" );
		status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum out of view too often. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
		
		// Check that we got a reasonable amount of movement.
		apparatus->ShowStatus(  "Checking for movement ...", "wait.bmp" );
		status = apparatus->CheckMovementAmplitude( ( eyes == CLOSED ? 1.0 : discreteMinMovementExtent ), 
													( eyes == CLOSED ? 1000.0 : discreteMaxMovementExtent ), 
													discreteMovementDirection, "Movement amplitude out of range. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that we got a reasonable number of movements. 
		// We expect as many as there are items in the sequence. 
		// But this post hoc test was designed for the oscillations protocol. 
		// So we will have half as many cycles as the number of expected movements.
		int tolerance = 5;
		int fewest = delaySequenceN / 2 - tolerance;
		if (fewest < 1) fewest = 1;
		int most = delaySequenceN / 2 + tolerance;
		if ( most < 1 ) most = 1;

		apparatus->ShowStatus(  "Checking for number of movements ...", "wait.bmp" );
		status = apparatus->CheckMovementCycles( fewest, most, discreteMovementDirection, discreteCycleHysteresis, "Not as many movements as we expected. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

#if 0
		// Did the subject anticipate the starting signal too often?
		apparatus->ShowStatus(  "Checking for early starts ...", "wait.bmp" );
		status = apparatus->CheckEarlyStarts( discreteFalseStartTolerance, discreteFalseStartHoldTime, 
			discreteFalseStartThreshold, discreteFalseStartFilterConstant, 
			"Too many early starts. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
#endif

		apparatus->ShowStatus(  "Analysis completed.", "ok.bmp" );
	}
	
	return( NORMAL_EXIT );
	
}



