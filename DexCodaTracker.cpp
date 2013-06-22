/***************************************************************************/
/*                                                                         */
/*                               DexCodaTracker                            */
/*                                                                         */
/***************************************************************************/

// A tracker that uses the Coda system.
// NB This uses the old Coda SDK. For the current system use DexRTnetTracker.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <codasys.h>
#include <CodaUtilities.h>

#include <VectorsMixin.h>
#include "DexTracker.h"

/***************************************************************************/

void DexCodaTracker::Initialize( void ) {

	// Initialize the CODA hardware.
	CodaConnectStartAndPrepare();
	samplePeriod = 0.005;	// Assuming the default for CodaConnectStartAndPrepare();

	// Prepare for acquiring single frames of marker data.
	coda_data_frame.dwChannelStart = 0;
	coda_data_frame.dwNumChannels = nMarkers;
	coda_data_frame.pData = fPosition;
	coda_data_frame.pValid = bInView;

	// Prepare for acquiring a continuous stream of marker data.
	coda_multi_acq_frame.bAutoBufferUpdate = 1;
	coda_multi_acq_frame.dwChannelStart = 0;
	coda_multi_acq_frame.dwNumChannels = nMarkers;
	coda_multi_acq_frame.dwBufferFrames = DEX_MAX_MARKER_FRAMES;
	coda_multi_acq_frame.dwFrameStart = 0;
	coda_multi_acq_frame.dwNumFrames = 0;
	coda_multi_acq_frame.dwUnit = 0;
	coda_multi_acq_frame.pData = fPositionMulti;
	coda_multi_acq_frame.pValid = bInViewMulti;

	// How many CODA units are there?
	// It would be nice if we could read this value.
	nCodas = N_CODAS;

}

int DexCodaTracker::Update( void ) {

	// The Coda tracker does not need any periodic updating.
	return( 0 );

}

/*********************************************************************************/

void DexCodaTracker::StartAcquisition( float max_duration ) {
	
	overrun = false;
	// Initiate continuous acquisition of marker information by the CODAs.
	// Up to nFrames will be acquired at a fixed rate.
	CodaAcqStart( DEX_MAX_MARKER_FRAMES );
	
}

bool DexCodaTracker::CheckAcquisitionOverrun( void ) {
	
	// Returns whether or not the previous acquisition overran the 
	// defined acquisition time limit.
	return( overrun );

}

void DexCodaTracker::StopAcquisition( void ) {

	// Assuming that we only do a StopAcquisition if an acquisition was started,
	// then the fact that an acquisition is no longer running means that the 
	// buffer got full and further acquisition was halted.
	overrun = !CodaAcqInProgress();
	
	// Stop the continuous acqusition.
	CodaAcqStop();
	
	// Determine how many frames were actually acquired.
	nAcqFrames = CodaAcqGetNumFramesMarker();
	if (CodaAcqGetMultiMarker(&coda_multi_acq_frame) == CODA_ERROR)
	{
		MessageBox( NULL, "Could not get CODA data.", "DEX", MB_OK );
		CodaQuit();
		exit( 0 );
	}
	
	// Output to the console some information about when the markers were visible or not>
	// For debugging purposes only.
	for ( int mrk = 0; mrk < nMarkers; mrk++ ) {
		fprintf( stderr, "Marker: %2d  ", mrk );
		for ( int i = 0; i < nAcqFrames; i += 200 ) {
			if ( bInViewMulti[i * nMarkers + mrk] ) fprintf( stderr, "0" );
			else fprintf( stderr, "." );
		}
		fprintf( stderr, "\n" );
	}
}

bool DexCodaTracker::GetCurrentMarkerFrame( CodaFrame &frame ) {
	
	// Request a single frame of data from CODA system. 
	if ( CODA_OK == CodaFrameGrab() ) {
		
		// This version takes the combined data from all available
		// CODAs and outputs all that is available.
		if ( CODA_OK == CodaFrameGetMarker( &frame ) ) {
			frame.time = 0.0;
			for ( int mrk = 0; mrk < nMarkers; mrk++ ) {
				frame.marker[mrk].visibility = ( bInView[mrk] != 0 ? true : false );
				CopyVector( frame.marker[mrk].position, &fPosition[mrk * 3] );
			}
		}
		return( true );
		
	}
	
	else return( false );
}

int DexCodaTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) {
	
	
	// Copy the acquired time series of manipulandum positions into an array.
	// Missing values, i.e. when the marker was not visible, are indicate by the INVISIBLE flag.
	// Again, we are using only a single marker to represent the position of the manipulandum.
	// In reality, we will use all markers on the manipulandum to define its position and orientation.
	for ( int frm = 0; frm < nAcqFrames && frm < max_frames; frm++ ) {
		frames[frm].time = frm * samplePeriod;
		for ( int mrk = 0; mrk < nMarkers; mrk++ ) {
			if ( bInViewMulti[ frm * nMarkers + mrk ] ) {
				for ( int k = 0; k < 3; k++ ) frames[frm].marker[mrk].position[k] = fPositionMulti[( frm * nMarkers + mrk ) * 3 + k];
				frames[frm].marker[mrk].visibility = true;
			}
			else {
				for ( int k = 0; k < 3; k++ ) frames[frm].marker[mrk].position[k] = INVISIBLE;
				frames[frm].marker[mrk].visibility = false;
			}
		}
	}
	return( frm );
}


bool DexCodaTracker::GetAcquisitionState( void ) {
	// I believe that this should work, but I did not test it with a CODA.
	return( ( CodaAcqInProgress() ? true : false ) );
}

int DexCodaTracker::GetNumberOfCodas( void ) {
	// Should return the actual number of active codas.
	// Here I set a fixed number.
	return( 2 );
}

