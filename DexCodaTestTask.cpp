/*********************************************************************************/
/*                                                                               */
/*                                DexCodaTestTask.cpp                            */
/*                                                                               */
/*********************************************************************************/

/*
 * To test the linearity of the CODA tracker we ask an operator to move the target
 * mast around while we record the position of the reference markers. Variations
 * in the distance between two fixed markers is an indication of the linearity.
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

int PrepCodaTest( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	
	DexTargetBarConfiguration bar_position = TargetBarRight;
	DexSubjectPosture posture = PostureSeated;

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Instruct the subject on the task to be done.
	AddDirective( apparatus, "To verify the accuracy of the tracker, you will perform a test using the Target Mast.", "baton_ready.bmp" );
	AddDirective( apparatus, "You will start with the Target Mast in socket N (right side) with Tapping Surfaces folded.", "baton_ready.bmp" );
	AddDirective( apparatus, "You will then remove the Target Mast from the socket and rotate it +/- 45° like a baton.", "baton_roll.bmp" );
	AddDirective( apparatus, "In another trial you will rotate the Target Mast in pitch, as instructed.", "baton_pitch.bmp" );
	AddDirective( apparatus, "Movements should be slow, with one full cycle per beep.", "baton_beep.bmp" );
	AddDirective( apparatus, "Reference markers should face toward Tracking Cameras, Target LEDs toward chair.", "alert.bmp" );
	AddDirective( apparatus, "You can remove the wrist strap, if you wish, to perform this task.", "BeltsSeated.bmp" );

	ShowDirectives( apparatus );

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunCodaTest( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;
	
	double frequency = 1.0;
	double period = 1.0;
	double duration = 10.0;
	double expected_cycles;

	Vector3 direction_vector = { 1.0, 0.0, 0.0 };
	char *direction_prompt = "";
	char *direction_picture = "blank.bmp";
	char *direction_picture2 = "blank.bmp";

	fprintf( stderr, "     RunCodaTest: %s\n", params );

	// How quickly should it oscillate?
	frequency = ParseForFrequency( apparatus, params );
	duration = ParseForDuration( apparatus, params );
	period = 1.0 / frequency;
	expected_cycles = frequency * duration;

	// Which direction?
	if ( ParseForBool( params, "-roll" ) ) {
		direction_prompt = "ROLL";
		direction_picture = "baton_roll.bmp";
		direction_picture2 = "baton_roll_beep.bmp";
	}
	if ( ParseForBool( params, "-pitch" ) ) {
		direction_prompt = "PITCH";
		direction_picture = "baton_pitch.bmp";
		direction_picture2 = "baton_pitch_beep.bmp";
	}

	// Construct the results filename tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else strcpy( tag, "CodaCHK" );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) status = PrepCodaTest( apparatus, params );

	// Indicate to the subject that we are ready to start and wait for their go signal.
	status = apparatus->WaitSubjectReady( "baton_ready.bmp", "Place Target Mast in Socket N on right. Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Start acquisition and acquire a baseline.
	// Presumably the mast is not in the hand. 
	apparatus->SignalEvent( "Initiating coda test movements." );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Instruct subject to take the manipulandum.
	//  and wait for confimation that he or she is ready.

	status = apparatus->WaitSubjectReady( "baton.bmp", "Remove Target Mast from socket and hold steady in hand. Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	// Indicate what to do next.
	status = apparatus->fWaitSubjectReady( direction_picture, "Perform %s oscillatory movements as shown in time with beeps.\nPress OK to start.", direction_prompt );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->ShowStatus( "Continue to oscillate in time with beeps.", direction_picture2 );

	// Mark the starting point in the recording where post hoc tests should be applied.
	// We do the analysis only on the part of the recorded movement that occurs after
	//  the beeps stops in order to detect if the subject mistakenly stopped as well.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Output a sound pattern to establish the oscillation frequency.
	double time;
	for ( time = 0.0; time < duration; time += period ) {
		int tone = 3.9; //floor( 3.9 * ( sin( time * 2.0 * PI ) + 1.0 ) );
		apparatus->SetSoundState( tone, 1 );
		apparatus->Wait( period / 2.0 - 0.01 );
		apparatus->SetSoundState( tone, 0 );
        apparatus->Wait( period / 2.0 - 0.01 );
	}
	apparatus->SetSoundState( 4, 0 );
	apparatus->MarkEvent( END_ANALYSIS );

	// Indicate to the subject that they are done and that they can set down the maniplulandum.
	SignalEndOfRecording( apparatus );
	status = apparatus->WaitSubjectReady( "baton_ready.bmp", "Replace Target Mast in Socket N (right side). Press OK." );
	if ( status == ABORT_EXIT ) exit( status );
	
	// Take a couple of seconds of extra data with the mast in the socket so we get another reference measurement.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Stop acquiring.
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	apparatus->SignalEvent( "Acquisition completed." );
	apparatus->StopFilming();
	apparatus->StopAcquisition( "Error during saving." );

	if ( ParseForBool( params, "-done" ) ) {
		status = apparatus->WaitSubjectReady( "BarLeft.bmp", "Place Target Mast in Standby socket on left. Press OK to continue." );
		if ( status == ABORT_EXIT ) exit( status );
		status = apparatus->WaitSubjectReady( "BeltsSeated.bmp", "If you removed the wrist strap, please don it again now. " );
		if ( status == ABORT_EXIT ) exit( status );
	}

	return( NORMAL_EXIT );
	
}