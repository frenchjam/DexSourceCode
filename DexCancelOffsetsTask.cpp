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

	status = apparatus->WaitSubjectReady( "Place manipulandum in holder.\n\n  !!! REMOVE HAND !!!\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	// Acquire some data.
	ShowStatus( "Acquiring offsets ..." );
	apparatus->StartAcquisition( maxTrialDuration );
	apparatus->Wait( offsetAcquireTime );
	apparatus->StopAcquisition();
	ShowStatus( "Saving data ..." );
	apparatus->SaveAcquisition( "OFFS" );
	ShowStatus( "Processing data ..." );
	// Compute the offsets and insert them into force calculations.
	apparatus->ComputeAndNullifyStrainGaugeOffsets();
	HideStatus();

	return( NORMAL_EXIT );

}


