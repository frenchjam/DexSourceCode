/*********************************************************************************/
/*                                                                               */
/*                          DexCancelOffsetsTask.cpp                             */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs the operations to measure and nullify the offets on the ATI strain gauges.
 */

/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

// Force offset parameters.
double offsetAcquireTime = 2.0;			// How long of a sample to acquire when computing strain gauge offsets.

/*********************************************************************************/

int RunTransducerOffsetCompensation( DexApparatus *apparatus, const char *params ) {

	int status;

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );
	apparatus->Comment( "Operation to cancel force sensor offsets." );
	apparatus->SignalEvent( "Preparing for force offset acquisition ..." );

	// Perhaps we could combine the next two instructions into a single one. Would need to change the picture.
	status = apparatus->fWaitSubjectReady( "RetainerManip.bmp", "Place manipulandum in the retainer.%s", OkToContinue );
	if ( status == ABORT_EXIT ) return( status );
	
	status = apparatus->fWaitSubjectReady( "REMOVE_HAND.bmp", "Remove hand during sensor offset measurement.%s", OkToContinue );
	if ( status == ABORT_EXIT ) return( status );

	// Acquire some data.
		apparatus->ShowStatus( "Acquiring offsets ...", "wait.bmp" );
	apparatus->StartAcquisition( "OFFS", maxTrialDuration );
	apparatus->Wait( offsetAcquireTime );
	apparatus->ShowStatus( "Saving data ..." );
	apparatus->StopAcquisition();

	apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
	// Compute the offsets and insert them into force calculations.
	apparatus->ComputeAndNullifyStrainGaugeOffsets();
	apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );
	BlinkAll( apparatus );
	apparatus->Wait( 1.0 );
	apparatus->HideStatus();

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );

	return( NORMAL_EXIT );

}


