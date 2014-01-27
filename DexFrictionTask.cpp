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
double slipWait = 0.5;
int    slipMovements = 5;

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
	status = apparatus->WaitSubjectReady( "pinch.bmp", "Squeeze the manipulandum with the thumb and the\n index finger centered.\nAdjust pinch force according to LED's.\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );
	status = apparatus->WaitCenteredGrip( copTolerance, copMinForce, copCheckTimeout, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );
    apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, frictionTimeout, "Use the LEDs to achieve the desired grip force level." );
	// Mark when the desired force is achieved.
	apparatus->MarkEvent( FORCE_OK );

	// Instruct the subject to perform the rubbing motion. 
	// !JMc I think that these instructions have to occur prior to achieving the desired force.
	// !JMc Otherwise, I think that the subject will lose the force level while reading the new instructions.
	status = apparatus->WaitSubjectReady( "Coef_frict_osc.bmp","Rub the manipulandum from center to periphery\n without releasing the grip.\n\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	// Start acquiring.
    apparatus->StartAcquisition( "FRIC", maxTrialDuration );

	// Wait for the initial slip.
    status = apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
			frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
			forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Mark when slip has occured. Note that if the subject hit <IGNORE> 
	// this signal will also occur.
	apparatus->MarkEvent( SLIP );

	// Allow 15 more seconds for the rubbing motion.
	apparatus->Wait( 15 );
	
	// !JMc Not sure what sound would be on.
	// !JMc Maybe this could be removed.
	apparatus->SoundOff();

	// Beep to indicate the end of the recording.
	apparatus->Beep();

	// Terminate the acquisition. This will also close the data file.
	apparatus->StopAcquisition();

	// Erase any lingering message.
	apparatus->HideStatus();

	// Apparently we were successful.
	return( NORMAL_EXIT );

}




