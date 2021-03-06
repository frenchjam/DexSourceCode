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

	fprintf( stderr, "     RunTransducerOffsetCompensation: %s\n", params );

	// Construct the file tag.
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else strcpy( tag, "GripOFFS" );

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );
	apparatus->Comment( "Operation to cancel force sensor offsets." );
	apparatus->SignalEvent( "Preparing for force offset acquisition ..." );
	apparatus->ShowStatus( "Preparing for acquisition.", "wait.bmp" );

	// Actually, the manipulandum is in the retainer only for friction test. So, as JL and PL suggested it, we could insert only one picture 
	// with the manipulandum in a cradle for the offsetMeasurement and another for friction.
	// The offsets are cancelled only once, just after installation and just before the friction test. 
	// So it makes more sense to have the manipulandum in the retainer.
	status = apparatus->fWaitSubjectReady( "REMOVE_HAND.bmp", "Remove hand for sensor offset measurement. Press <OK> to start." );
	if ( status == ABORT_EXIT ) return( status );
	
	status = apparatus->SelfTest();
	if ( status == ABORT_EXIT ) return( status );

	// Acquire some data.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->Wait( offsetAcquireTime );
	apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
	apparatus->StopAcquisition( "Error during file save." );

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


