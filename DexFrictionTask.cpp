/*********************************************************************************/
/*                                                                               */
/*                             DexFrictionTask.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs the operations to measure the coeficient of friction.
 */

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

// Coefficient of friction test parameters.
double frictionHoldTime = 3.0; //en seconde
double frictionTimeout = 10.0;

// GF=0.5N 3x, 1N 2x, 2N 2x, 4N 1x
double gripTarget= 0.5; //0.5 or 1.0 or 2.0 or 4.0

// Define the pull direction. This should be up.
Vector3 frictionLoadDirection = { 0.0, 1.0, 0.0 };
// Define the filter constant for the visual force feedback.
// A larger value reduces the jitter but also makes the 
//  response sluggish.
double forceFilterConstant = 4.0;
// Use the method of counting slips vs. a fixed number of seconds depending on the desired grip force.
// This is a function of the default threshold for the COP calculation, which is settable in ASW.ini.
// In the end, though, we will probably choose one method or the other.
double frictionMethodThreshold = 0.25;

double slipThreshold = 5.0;		// How far the COP must move to be considered a slip.
double slipTimeout = 10.0;		// How long should we wait for a slip?
double slipWait = 0.25;
int    slipMovements = 15;

/*********************************************************************************/

int PrepFrictionMeasurement( DexApparatus *apparatus, const char *params ) {

	double duration = 15.0;
	duration = ParseForDuration( apparatus, params );
	char msg[1024];

	AddDirective( apparatus, "You will first pinch the manipulandum at the center between thumb and forefinger.", "coef_frict.bmp" );
	AddDirective( apparatus, "Squeeze according to the instructions that you will receive (firmly, moderately, lightly).", "coef_frict.bmp" );
	AddDirective( apparatus, "When you hear the beep, rub the fingers up and down on the manipulandum. ", "coef_frict_osc.bmp");
	sprintf( msg, "You will rub for %.0f seconds. Move from center to edge and back. Try to maintain pinch force.", duration );
	AddDirective( apparatus, msg, "rub.bmp"  );
	ShowDirectives( apparatus );

	return( NORMAL_EXIT );

}

int RunFrictionMeasurement( DexApparatus *apparatus, const char *params ) {
	
	int status;
	char *squeeze;

	// DexNiDaqADC must be run in polling mode so that we can record the 
	// continuous data, but also monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	if ( apparatus->adc ) apparatus->adc->AllowPollingDuringAcquisition();

	gripTarget = ParseForPinchForce( apparatus, params );
	double frictionMinGrip = gripTarget - 0.25;
	if ( frictionMinGrip < 0.1 ) frictionMinGrip = 0.1;
	double frictionMaxGrip = gripTarget + 2.0;
	if ( frictionMinGrip > 10.0 ) frictionMinGrip = 10.0;
	double frictionMinLoad = 0.0;
	double frictionMaxLoad = 0.0;
	forceFilterConstant = ParseForFilterConstant( apparatus, params );
	double threshold = min( copForceThreshold, gripTarget * 0.9 );


	fprintf( stderr, "     RunFrictionMeasurement: %s\n", params );
	
	if ( ParseForPrep( params ) ) status = PrepFrictionMeasurement( apparatus, params );

	// Construct the file tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else {
		sprintf( tag, "GripF%.0fp%.0f", floor( gripTarget ), (gripTarget - floor( gripTarget )) * 10.0 );
	}

	double duration = 15.0;
	duration = ParseForDuration( apparatus, params );

	if ( gripTarget < 0.75 ) squeeze = "LIGHTLY";
	else if ( gripTarget > 1.5 ) squeeze = "FIRMLY";
	else squeeze = "MODERATELY";

    status = apparatus->fWaitSubjectReady( "Coef_frict.bmp", "On this trial you will squeeze %s.", squeeze  );
	if ( status == ABORT_EXIT ) exit( status );

    // picture Remove Hand with manipulandum in the retainer.
	apparatus->WaitSubjectReady( "REMOVE_HAND.bmp", "*****     PREPARING TO START     *****\nRemove hand from the manipulandum." );

	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Start acquiring.
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );


    apparatus->fShowStatus( "Coef_frict.bmp", "Pinch the manipulandum %s at the center between thumb and index finger.", squeeze  );
	apparatus->Beep();

	status = apparatus->WaitCenteredGrip( 20.0, copForceThreshold, copWaitTime, "Manipulandum not in hand \n      Or      \n Fingers not centered.", "alert.bmp" );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->fShowStatus( "rub.bmp", "Squeeze %s and rub up and down for %.0f seconds.", squeeze, duration );

	apparatus->Wait( duration );

	apparatus->Beep();
	apparatus->Beep();
	BlinkAll( apparatus );
	apparatus->WaitSubjectReady( "REMOVE_HAND.bmp", MsgReleaseAndOK );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Collect some data with zero force.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Terminate the acquisition. This will also close the data file.
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	SignalEndOfRecording( apparatus );
	apparatus->StopFilming();
	apparatus->StopAcquisition( "Error during file save." );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( NULL, "Task completed normally." );
	if ( status == ABORT_EXIT ) exit( status );

	// Apparently we were successful.
	return( NORMAL_EXIT );

}




