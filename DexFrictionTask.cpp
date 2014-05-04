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
double frictionMethodThreshold = 0.45;

double slipThreshold = 5.0;		// How far the COP must move to be considered a slip.
double slipTimeout = 10.0;		// How long should we wait for a slip?
double slipWait = 0.25;
int    slipMovements = 15;

/*********************************************************************************/



int RunFrictionMeasurement( DexApparatus *apparatus, const char *params ) {
	
	int status;

	// DexNiDaqADC must be run in polling mode so that we can record the 
	// continuous data, but also monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	if ( apparatus->adc ) apparatus->adc->AllowPollingDuringAcquisition();

	gripTarget = ParseForPinchForce( apparatus, params );
	double frictionMinGrip = 0.85 * gripTarget;
	double frictionMaxGrip = 1.25 * gripTarget;
	double frictionMinLoad = 0.0;
	double frictionMaxLoad = 0.0;
	forceFilterConstant = ParseForFilterConstant( apparatus, params );
	double threshold = min( copForceThreshold, gripTarget * 0.9 );

	fprintf( stderr, "     RunFrictionMeasurement: %s\n", params );
	
	if ( ParseForPrep( params ) ) {

        status = apparatus->WaitSubjectReady("OpenRetainer.bmp", "Deploy the retainer." );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
		status = apparatus->WaitSubjectReady("RetainerManip.bmp", "Lock the manipulandum into the retainer." );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

    // picture Remove Hand with manipulandum in the retainer.
	apparatus->WaitSubjectReady( "REMOVE_HAND.bmp", "Remove hand from the manipulandum and press <OK> to start." );

	// Start acquiring.
	apparatus->StartAcquisition( "FRIC", maxTrialDuration );
	apparatus->ShowStatus( "Acquiring baseline ...", "wait.bmp" );
	apparatus->Wait( baselineDuration );
	

    apparatus->ShowStatus( "Grip the manipulandum at the center, (1) adjust the grip force according to the LED's and then (2) rub up and down.", "pinch.bmp" );
    
	// Light the targets that will be used to adjust the grip force.
	// At first they will be on statically, until we detect a grip.

	apparatus->TargetsOff();
	apparatus->TargetOn( 6 );
	apparatus->TargetOn( 10 );
	apparatus->TargetOn( 0 );

	status = apparatus->WaitCenteredGrip( copTolerance, copForceThreshold, copWaitTime, "Manipulandum not in hand \n      Or      \n Fingers not centered.", "alert.bmp" );
	if ( status == ABORT_EXIT ) exit( status );


   status = apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
  		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
  		forceFilterConstant, frictionHoldTime, frictionTimeout, "Desired grip force was not achieved.", "alert.bmp" );


	// Mark when the desired force is achieved.
	if ( status == ABORT_EXIT ) exit( status );
	apparatus->MarkEvent( FORCE_OK );

	// Rub the manipulandum
	apparatus->ShowStatus( "Grip adjusted! Rub the manipulandum up and down during 15 sec without releasing the grip.", "rub.bmp" );

	// Wait for the initial slip.
    status = apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
			frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
			forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Mark when slip has occured. Note that if the subject hit <IGNORE> 
	// this signal will also occur.
	apparatus->MarkEvent( SLIP );

	// Choose the method depending on the desired grip force.
	if ( frictionMinGrip < frictionMethodThreshold ) {

		// This is the old version, based on a fixed amount of time for the rubbing motions.
		// Allow 15 more seconds for the rubbing motion.

		for ( int i = 0; i <= 10; i++ ) {
			AnalysisProgress( apparatus, i, 10, "Keep rubbing." );
			apparatus->Wait( 1.5 );
		}
		// !JMc Not sure what sound would be on.
		// !JMc Maybe this could be removed.
		apparatus->SoundOff();

	}
	else {

		// In this version we wait for a certain number of slips to be detected. 
		// There is a small delay between each call to WaitSlip() with the hopes that
		//  the same slip will not be detected twice, but even that should not be a problem.
		for ( int slip = 0; slip < slipMovements; slip++ ) {
			AnalysisProgress( apparatus, slip, slipMovements, "Keep rubbing. Need more slips." );
			status = apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
					frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
					forceFilterConstant, slipThreshold, slipTimeout, "Not enough slips achieved.\n(<Ignore> to keep trying.)", "alert.bmp"  );
			if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
			apparatus->MarkEvent( SLIP );
		}
		AnalysisProgress( apparatus, slipMovements, slipMovements, "Success!" );

	}

	apparatus->Beep();
	BlinkAll( apparatus );
	apparatus->WaitSubjectReady( "REMOVE_HAND.bmp", "Remove hand and press <OK> to continue." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Collect some data with zero force.
	apparatus->ShowStatus( "Acquiring baseline ...", "wait.bmp" );
	apparatus->Wait( baselineDuration );

	// Terminate the acquisition. This will also close the data file.
	apparatus->StopAcquisition();
	SignalEndOfRecording( apparatus );

	// Erase any lingering message.
	apparatus->HideStatus();

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );

	// Apparently we were successful.
	return( NORMAL_EXIT );

}




