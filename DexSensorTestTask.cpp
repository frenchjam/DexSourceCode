/*********************************************************************************/
/*                                                                               */
/*                              DexSensorTestTask.cpp                            */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs a set of oscillatory movements while holding the manipuladum by ends.
 * Correlation between acceleration and force data is used to validate the sensors.
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

double stOscillationMinMovementExtent = 19.0;		// Minimum amplitude along the movement direction (nominally Y). Set to 1000.0 to simulate error.
double stOscillationMaxMovementExtent = 1000.0;	// Maximum amplitude along the movement direction. Set to 1.0 to simulate error.

double stOscillationCycleHysteresis = 10.0;		// Parameter used to adjust the detection of cycles. 

/*********************************************************************************/

int PrepSensorTest( DexApparatus *apparatus, const char *params ) {

	int status = 0;
	

	status = apparatus->WaitSubjectReady( "PlaceAndFold.bmp", "If necessary, place maniplulandum in any cradle and stow retainer. Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "BarLeft.bmp", "If necessary, move Target Mast to Standby position (left side). Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Instruct the subject on the task to be done.
	AddDirective( apparatus, "You will move the manipulandum in a specific manner to test the sensors.", "diagnostics.bmp" );
	AddDirective( apparatus, "The manipulandum must be held in a non-standard fashion as shown, with front face forward.", "SensorTestGrip.bmp" );
	AddDirective( apparatus, "Movements of the hand will be in one of 3 directions according to instructions.", "SensorTestAll.bmp" );
	AddDirective( apparatus, "Movements should be continuous oscillations (sinusoids), with one full cycle per beep.", "OscillateST.bmp" );
	AddDirective( apparatus, "Movements amplitude should be ~50 cm. (Workspace Tablet edges can be used as a reference.)", "dimensions.bmp" );
	ShowDirectives( apparatus );

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunSensorTest( DexApparatus *apparatus, const char *params ) {
	
	int status = 0;
	
	double frequency = 1.0;
	double period = 1.0;
	double duration = 10.0;
	double expected_cycles;

	Vector3 direction_vector = { 1.0, 0.0, 0.0 };
	char *direction_prompt = "SIDE-TO-SIDE";
	char *direction_picture = "SensorTestTrans.bmp";

	fprintf( stderr, "     RunSensorTest: %s\n", params );

	// How quickly should it oscillate?
	frequency = ParseForFrequency( apparatus, params );
	duration = ParseForDuration( apparatus, params );
	period = 1.0 / frequency;
	expected_cycles = frequency * duration;

	// Which direction?
	if ( ParseForBool( params, "-vertical" ) ) {
		direction_prompt = "UP-AND-DOWN";
		apparatus->CopyVector( direction_vector, apparatus->jVector );
		direction_picture = "SensorTestVert.bmp";
	}
	if ( ParseForBool( params, "-horizontal" ) || ParseForBool( params, "-depth" ) ) {
		direction_prompt = "FRONT-TO-BACK";
		apparatus->CopyVector( direction_vector, apparatus->kVector );
		direction_picture = "SensorTestHoriz.bmp";
	}
	if ( ParseForBool( params, "-sideways" ) ) {
		direction_prompt = "SIDE-TO-SIDE";
		apparatus->CopyVector( direction_vector, apparatus->iVector );
		direction_picture = "SensorTestTrans.bmp";
	}


	// Construct the results filename tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else strcpy( tag, "SensrCHK" );

	// If told to do so in the command line, give the subject explicit instructions to prepare the task.
	// If this is the first block, we should do this. If not, it can be skipped.
	if ( ParseForPrep( params ) ) status = PrepSensorTest( apparatus, params );

	// Verify that the apparatus is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	DexTargetBarConfiguration bar_position = TargetBarLeft;
	DexSubjectPosture posture = PostureSeated;
	status = CheckInstall( apparatus, posture, bar_position );
	if ( status != NORMAL_EXIT ) return( status );

	// Indicate to the subject that we are ready to start and wait for their go signal.
	status = apparatus->WaitSubjectReady( "HandsOffCradle.bmp", MsgReadyToStart );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Start acquisition and acquire a baseline.
	// Presumably the manipulandum is not in the hand. 
	// It should have been left either in a cradle or the retainer at the end of the last action.
	apparatus->SignalEvent( "Initiating sensor test movements." );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Instruct subject to take the manipulandum.
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "TakeEmpty.bmp", "Pick up manipulandum, leaving mass in cradle. Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "SensorTestGrip.bmp", "Take manipulandum between fingers as shown and hold steady. Press OK to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineDuration );
	
	// Indicate what to do next.
	status = apparatus->fWaitSubjectReady( direction_picture, "Perform %s oscillatory movements as shown in time with beeps.\nPress OK to start.", direction_prompt );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->ShowStatus( "Continue to oscillate in time with beeps.", "OscillateST.bmp" );

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
	status = apparatus->WaitSubjectReady( "PlaceMassNoSlide.bmp", MsgReadyToStart );
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
		int min_cycles = expected_cycles - 5;
		if ( min_cycles < 1 ) min_cycles = 1;
		int max_cycles = (int)((float) expected_cycles * 1.5f);
		char msg[1024];

		// Was the manipulandum obscured?
		apparatus->ShowStatus( "Checking visibility ...", "wait.bmp" );
		status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum occluded too often. Press <Retry> to repeat (once or twice) or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that we got a reasonable amount of movement.
		apparatus->ShowStatus( "Checking for movement ...", "wait.bmp" );
		sprintf( msg, "Movement amplitude out of range.\n- Movements were %s?\n<Retry> to repeat or call COL-CC.", direction_prompt );
		status = apparatus->CheckMovementAmplitude( stOscillationMinMovementExtent, stOscillationMaxMovementExtent, direction_vector, msg, "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that we got a reasonable number of oscillations.
		apparatus->ShowStatus( "Checking number of Oscillations ...", "wait.bmp" );
		sprintf( msg, "Number of oscillations out of range.\n- Movements were %s?\n<Retry> to repeat or call COL-CC.", direction_prompt );
		status = apparatus->CheckMovementCycles( min_cycles, max_cycles, direction_vector, stOscillationCycleHysteresis, msg, "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		apparatus->ShowStatus(  "Analysis completed.", "ok.bmp" );
	}

	return( NORMAL_EXIT );
	
}