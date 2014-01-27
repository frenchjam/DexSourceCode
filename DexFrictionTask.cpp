/*********************************************************************************/
/*                                                                               */
/*                             DexFrictionTask.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs the operations to measure the coeficient of friction.
 */

#include <windows.h>
#include <mmsystem.h>s

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

	// Start acquiring Data.
	// Note: DexNiDaqADC must be run in polling mode so that we can both record the 
	// continuous data, but all monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	apparatus->adc->AllowPollingDuringAcquisition();


	status = apparatus->WaitSubjectReady( "pinch.bmp", "Squeeze the manipulandum with the thumb and the\n index finger centered.\nAdjust pinch force according to LED's.\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	status = apparatus->WaitCenteredGrip( copTolerance, copMinForce, copCheckTimeout, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );


	// we still need to flip the horizontal and vertical bar for displaying the LED's as feedback for pinch force. 
    apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, frictionTimeout, "Use the LEDs to achieve the desired grip force level." );
	

	status = apparatus->WaitSubjectReady( "Coef_frict_osc.bmp","Rub the manipulandum from center to periphery\n without releasing the grip.\n\nPress <OK> when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

    apparatus->StartAcquisition( "FRIC", maxTrialDuration );
    apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
			frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
			forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
	apparatus->MarkEvent( SLIP_OK );

	apparatus->Wait( 15 );
	
	//Sound off
	apparatus->SoundOff();

	apparatus->MarkEvent( FORCE_OK );

	apparatus->Beep();

	apparatus->StopAcquisition();

	apparatus->HideStatus();

	return( NORMAL_EXIT );

}




