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
double frictionHoldTime = 2.0;
double frictionTimeout = 200.0;

// GF=0.5N 3x, 1N 2x, 2N 2x, 4N 1x
double gripTarget= 4.0; //0.5 or 1.0 or 2.0 or 4.0
double frictionMinGrip = 0.75*gripTarget;
double frictionMaxGrip = 1.25*gripTarget;
double frictionMinLoad = -20.0;
double frictionMaxLoad = 20.0;
double forceFilterConstant = 1.0;

// Define the pull direction. This should be up.
Vector3 frictionLoadDirection = { 0.0, 1.0, 0.0 };

double slipThreshold = 1.0;
double slipTimeout = 10.0;
double slipWait = 0.25;
int    slipMovements = 15;

double copMinForce = 1.0;
double copTimeout = 10.0;
double copCheckTimeout = 1.0;

/*********************************************************************************/



int RunFrictionMeasurement( DexApparatus *apparatus, const char *params ) {
	
	int status;

	// DexNiDaqADC must be run in polling mode so that we can record the 
	// continuous data, but also monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	apparatus->adc->AllowPollingDuringAcquisition();

	// Instruct the subject to achieve the desired grip center and force, then wait until it is achieved.
	GiveDirective( apparatus, "You will first grasp the manipulandum with\nthumb and index finger centered, while the\nmanipulandum remains in the retainer.", "pinch.bmp" );
	GiveDirective( apparatus, "Squeeze the manipulandum with the thumb and the\n index finger centered.\nAdjust pinch force according to LED's.", "pinch.bmp" );
	GiveDirective( apparatus, "Rub the manipulandum from center to periphery\n without releasing the grip.", "Coef_frict_osc.bmp" );
	
	// Start acquiring.
    apparatus->StartAcquisition( "FRIC", maxTrialDuration );

	// Collect some data with zero force.
	apparatus->Wait( 2.0 );

    status = apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, frictionTimeout, "Use the LEDs to achieve the desired grip force level." );
	// Mark when the desired force is achieved.
	if ( status == ABORT_EXIT ) exit( status );
	apparatus->MarkEvent( FORCE_OK );

	status = apparatus->WaitCenteredGrip( copTolerance, copMinForce, copCheckTimeout, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	// Beep to let the subject know that he or she can start rubbing.
	apparatus->Beep();

	// Wait for the initial slip.
    status = apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
			frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
			forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Mark when slip has occured. Note that if the subject hit <IGNORE> 
	// this signal will also occur.
	apparatus->MarkEvent( SLIP );

#if 0
	// Allow 15 more seconds for the rubbing motion.
	apparatus->Wait( 15 );
#else
	for ( int slip = 0; slip < slipMovements; slip++ ) {
		status = apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
				frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
				forceFilterConstant, slipThreshold, slipTimeout, "Not enough slips achieved.", "alert.bmp"  );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
		apparatus->MarkEvent( SLIP );
		apparatus->Wait( slipWait );
	}
#endif


	
	// !JMc Not sure what sound would be on.
	// !JMc Maybe this could be removed.
	apparatus->SoundOff();

	// Beep to indicate the end of the recording.
	apparatus->Beep();

	// Let the subject know that they are done.
	BlinkAll( apparatus );
	BlinkAll( apparatus );

	apparatus->WaitSubjectReady( "ok.bmp", "Release the maniplandum and press <OK> to continue." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Collect some data with zero force.
	apparatus->Wait( 2.0 );

	// Terminate the acquisition. This will also close the data file.
	apparatus->StopAcquisition();

	// Erase any lingering message.
	apparatus->HideStatus();

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( NULL, "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );

	// Apparently we were successful.
	return( NORMAL_EXIT );

}




