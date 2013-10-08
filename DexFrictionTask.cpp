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

// These are easy, but probably too easy.
double frictionMinGrip = 2.0;
double frictionMaxGrip = 5.0;
double frictionMinLoad = -20.0;
double frictionMaxLoad = 20.0;
double forceFilterConstant = 1.0;

// Define the pull direction. This should be up.
Vector3 frictionLoadDirection = { 0.0, 0.0, 1.0 };

double slipThreshold = 100.0;
double slipTimeout = 10.0;

/*********************************************************************************/



int RunFrictionMeasurement( DexApparatus *apparatus, const char *params ) {
	
	int status;

	// Start acquiring Data.
	// Note: DexNiDaqADC must be run in polling mode so that we can both record the 
	// continuous data, but all monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	apparatus->adc->AllowPollingDuringAcquisition();
	
	status = apparatus->WaitSubjectReady( "Pictures\\Coef_frict.bmp", "Squeeze the manipulandum between thumb and index with the right hand.\nBe sure that the thumb and the forefinger are centered.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	status = apparatus->WaitCenteredGrip( copTolerance, 1.0, 1.0, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Pictures\\Coef_frict_osc.bmp","Rub the manipulandum with the \nadjusted pinch force according to LEDs.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

    apparatus->StartAcquisition( "FRIC", maxTrialDuration );
  
	//Sound off
	apparatus->SoundOff();

	apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, frictionTimeout, "Desired force not achieved." );
	apparatus->MarkEvent( FORCE_OK );

	//apparatus->Beep();
    
	//apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
	//	frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
	//	forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
	//apparatus->MarkEvent( SLIP_OK );
	//apparatus->HideStatus();

	apparatus->StopAcquisition();

	apparatus->HideStatus();

	return( NORMAL_EXIT );

}




