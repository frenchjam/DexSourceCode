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

	status = apparatus->WaitSubjectReady( "Pictures\\RetainerPhoto.bmp", "Place manipulandum in empty cradle.\n    !!! REMOVE HAND !!!\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	// Acquire some data.
	apparatus->ShowStatus( "Acquiring offsets ..." );
	apparatus->StartAcquisition( "OFFS", maxTrialDuration );
	apparatus->Wait( offsetAcquireTime );
	apparatus->ShowStatus( "Saving data ..." );
	apparatus->StopAcquisition();

	apparatus->ShowStatus( "Processing data ..." );
	// Compute the offsets and insert them into force calculations.
	apparatus->ComputeAndNullifyStrainGaugeOffsets();
	apparatus->ShowStatus( "Force offsets nullified." );

	apparatus->SignalNormalCompletion();

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );

	return( NORMAL_EXIT );

}


