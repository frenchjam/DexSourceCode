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
double frictionTimeout = 15.0;
double frictionMinGrip = 10.0;
double frictionMaxGrip = 15.0;
double frictionMinLoad = 5.0;
double frictionMaxLoad = 10.0;
double forceFilterConstant = 1.0;
Vector3 frictionLoadDirection = { 0.0, -1.0, 0.0 };
double slipThreshold = 10.0;
double slipTimeout = 20.0;

/*********************************************************************************/



int RunFrictionMeasurement( DexApparatus *apparatus, const char *params ) {
// 3 first targets output are used to drive a sound. When the 3 targets are turned on no sound appears otherwise
	// a sound is played.
	//initialize sound off
//    apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);
	
	int status;

	// Start acquiring Data.
	// Note: DexNiDaqADC must be run in polling mode so that we can both record the 
	// continuous data, but all monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	apparatus->adc->AllowPollingDuringAcquisition();
	

	status = apparatus->WaitSubjectReady( "Squeeze the manipulandum between thumb and index with the right hand.\nBe sure that the thumb and the forefinger are centered.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	status = apparatus->WaitCenteredGrip( copTolerance, 1.0, 1.0, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	status = apparatus->WaitSubjectReady( "Pull Upward.\nAdjust pinch and pull forces according to LEDs.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

    status = apparatus->WaitSubjectReady( "When you will hear the beep, pull harder until slippage.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

    apparatus->StartAcquisition( "FRIC", maxTrialDuration );
  

	
	
	
	//Sound off
//	apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);

	apparatus->SoundOff();

	apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, frictionTimeout, "Desired force not achieved." );
	apparatus->MarkEvent( FORCE_OK );
//    apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);
	//Sound On
	apparatus->Beep();
//	apparatus->TargetOff(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);
	//Sound off
//	apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);

    
	apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, slipThreshold, slipTimeout, "Slip not achieved."  );
//  apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);
	apparatus->MarkEvent( SLIP_OK );
	HideStatus();

//    apparatus->TargetOn(0);
//	apparatus->TargetOn(1);
//	apparatus->TargetOn(2);
	apparatus->StopAcquisition();

	HideStatus();

	return( NORMAL_EXIT );

}




