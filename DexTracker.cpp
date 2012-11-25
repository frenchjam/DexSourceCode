/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <useful.h>
#include <screen.h>
#include <3dMatrix.h>
#include <Timers.h>
#include "ConfigParser.h" 
#include <2dMatrix.h>
#include <Regression.h>

#include <CodaUtilities.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

#include "AfdObjects.h"
#include "CodaObjects.h"
#include "DexGlObjects.h"
#include "DexTracker.h"
#include "Dexterous.h"

/***************************************************************************/

DexTracker::DexTracker() {}

void DexTracker::Initialize( void ) {}
int DexTracker::Update( void ) {
	return( 0 );
}
void DexTracker::Quit() {}

void DexTracker::StartAcquisition( float max_duration ) {}
void DexTracker::StopAcquisition( void ) {}
int  DexTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) { 
	return( 0 );
}
bool  DexTracker::GetCurrentMarkerFrame( CodaFrame &frame ) { 
	return( false );
}

double DexTracker::GetSamplePeriod( void ) {
	return( samplePeriod );
}
int DexTracker::GetNumberOfCodas( void ) {
	return( 0 );
}
bool DexTracker::GetAcquisitionState( void ) {
	return( false );
}
bool DexTracker::CheckOverrun( void ) {
	return( false );
}


/***************************************************************************/

DexCodaTracker::DexCodaTracker( int n_codas, int n_markers, int n_frames ) {

	nCodas = n_codas;
	nMarkers = n_markers;
	nFrames = n_frames;

}

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
	coda_multi_acq_frame.dwBufferFrames = nFrames;
	coda_multi_acq_frame.dwFrameStart = 0;
	coda_multi_acq_frame.dwNumFrames = 0;
	coda_multi_acq_frame.dwUnit = 0;
	coda_multi_acq_frame.pData = fPositionMulti;
	coda_multi_acq_frame.pValid = bInViewMulti;


}

int DexCodaTracker::Update( void ) {

	// The Coda tracker does not need any periodic updating.
	return( 0 );

}

/*********************************************************************************/

void DexCodaTracker::StartAcquisition( float max_duration ) {

	// Initiate continuous acquisition of marker information by the CODAs.
	// Up to nFrames will be acquired at a fixed rate.
  CodaAcqStart( nFrames );

}

void DexCodaTracker::CheckAcquisitionOverrun( void ) {

	// Check if the acquisition is still running. 
	// If not, that means the acquisition number of frames is too small
	//  with respect to the task;
	if ( !CodaAcqInProgress() ) {
		MessageBox( NULL, "Acquisition overrun.", "DEX", MB_OK );
		CodaQuit();
		exit( 0 );
	}
}

void DexCodaTracker::StopAcquisition( void ) {

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

	frame.time = 0.0;
	for ( int j = 0; j < nMarkers; j++ ) {
		frame.marker[j].visibility = false;
		for ( int k = 0; k < 3; k++ ) {
			frame.marker[j].position[k] = 0.0;
		}
	}
	return( true );
}

int DexCodaTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) {

#if 0
	// Copy the acquired time series of manipulandum positions into an array.
	// Missing values, i.e. when the marker was not visible, are indicate by the INVISIBLE flag.
	// Again, we are using only a single marker to represent the position of the manipulandum.
	// In reality, we will use all markers on the manipulandum to define its position and orientation.
	for ( int i = 0, k = 0; i < nAcqFrames * nMarkers * 3; i += nMarkers * 3, k += 3 ) {
		if ( bInViewMulti[i] ) {
			for ( int j = 0; j < 3; j++ ) position[k + j] = fPositionMulti[i + j];
		}
		else {
			for ( int j = 0; j < 3; j++ ) recordedMarkerFrames[i + j] = INVISIBLE;
		}
	}
#endif
	return( nAcqFrames );
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


/***************************************************************************/
/*                                                                         */
/*                              DexMouseTracker                            */
/*                                                                         */
/***************************************************************************/

// This is a simulated tracker that allows us to work without a CODA.

DexMouseTracker::DexMouseTracker( void ) {

	nMarkers = 1;
	nFrames = 1000;
	samplePeriod = 0.5;
	acquisitionOn = false;
	overrun = false;

}

/*********************************************************************************/

void DexMouseTracker::Initialize( void ) {
	nAcqFrames = 0;
}

/*********************************************************************************/

void DexMouseTracker::StartAcquisition( float max_duration ) { 
	acquisitionOn = true; 
	overrun = false;
	DexTimerSet( acquisitionTimer, max_duration );
}

void DexMouseTracker::StopAcquisition( void ) {
	acquisitionOn = false;
}

int DexMouseTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) {

	// Copy data into an array.
	for ( int i = 0; i < nAcqFrames; i++ ) {
		frames[i].time = recordedMarkerFrames[i].time;
		for ( int j = 0; j < nMarkers; j++ ) {
			frames[i].marker[j].visibility = recordedMarkerFrames[i].marker[j].visibility;
			for ( int k = 0; k < 3; k++ ) {
				frames[i].marker[j].position[k] = recordedMarkerFrames[i].marker[j].position[k];
			}
		}
	}
	return( nAcqFrames );

}

bool DexMouseTracker::GetCurrentMarkerFrame( CodaFrame &frame ) {

	frame.time = currentMarkerFrame.time;
	for ( int j = 0; j < nMarkers; j++ ) {
		frame.marker[j].visibility = true;
		for ( int k = 0; k < 3; k++ ) {
			frame.marker[j].position[k] = currentMarkerFrame.marker[j].position[k];
		}
	}
	return( true );
}

bool DexMouseTracker::GetAcquisitionState( void ) {
	return( acquisitionOn );
}

bool DexMouseTracker::CheckOverrun( void ) {
	return( overrun );
}


/*********************************************************************************/

int DexMouseTracker::Update( void ) {

	POINT mouse_position;
	GetCursorPos( &mouse_position );
	RECT rect;
	GetWindowRect( GetDesktopWindow(), &rect );

	double z = (double)  -100 + (mouse_position.x - rect.right) / (double) ( rect.right - rect.left ) * 240.0;
	double y = (double)   mouse_position.y / (double) ( rect.bottom - rect.top ) * 240.0;

	// Marker 0 follows the mouse.
	currentMarkerFrame.marker[0].position[Z] = z;
	currentMarkerFrame.marker[0].position[Y] = y;
	currentMarkerFrame.marker[0].position[X] = 60.0;
	currentMarkerFrame.marker[0].visibility = true;

	// Other markers follow a sinusoid.
	for ( int j = 1; j < nMarkers; j++ ) {
		currentMarkerFrame.marker[j].visibility = true;
		for ( int k = 0; k < 3; k++ ) {
			currentMarkerFrame.marker[j].position[k] = cos( DexTimerElapsedTime( acquisitionTimer ) );
		}
	}

	// Store the time series of data.
	if ( DexTimerTimeout( acquisitionTimer ) ) {
		acquisitionOn = false;
		overrun =true;
	}
	if ( acquisitionOn && nAcqFrames < DEX_MAX_DATA_FRAMES ) {
		recordedMarkerFrames[ nAcqFrames ].time = DexTimerElapsedTime( acquisitionTimer );
		for ( int j = 0; j < nMarkers; j++ ) {
			recordedMarkerFrames[nAcqFrames].marker[j].visibility = currentMarkerFrame.marker[j].visibility;
			for ( int k = 0; k < 3; k++ ) {
				recordedMarkerFrames[nAcqFrames].marker[j].position[k] = currentMarkerFrame.marker[j].position[k]; 
			}
		}
		if ( nAcqFrames < nFrames ) nAcqFrames++;
	}
	return( 0 );

}

#if 0  // Ignore the Virtual Tracker for now.

/***************************************************************************/
/*                                                                         */
/*                            DexVirtualTracker                            */
/*                                                                         */
/***************************************************************************/

// This is a simulated tracker that allows us to work without a CODA.

DexVirtualTracker::DexVirtualTracker( DexTargets *targets ) {

	this->targets = targets;
	nMarkers = 1;
	nFrames = 1000;
	acquisition_on = false;

}

/*********************************************************************************/

void DexVirtualTracker::Initialize( void ) {

	TimerStart( &oscillate_timer );

	manipulandumPosition[X] = 150.0;
	manipulandumPosition[Y] = 125.0;
	manipulandumPosition[Z] = 25.0;

	samplePeriod = 0.5;

	nAcqFrames = 0;

}

/*********************************************************************************/

bool DexVirtualTracker::GetManipulandumPosition( float *pos, float *ori ) {

	pos[X] = manipulandumPosition[X];
	pos[Y] = manipulandumPosition[Y];
	pos[Z] = manipulandumPosition[Z];

	ori[X] = ori[Y] = ori[Z] = 0.0;
	ori[M] = 1.0;

	return( true );

}

bool DexVirtualTracker::GetFramePosition( float *pos ) {

	pos[X] = 0.0;
	pos[Y] = 0.0;
	pos[Z] = 0.0;

	return( true );

}

/*********************************************************************************/

void DexVirtualTracker::StartAcquisition( float max_duration ) { 
	acquisition_on = true; 
}
void DexVirtualTracker::StopAcquisition( void ) {
	acquisition_on = false;
}

int DexVirtualTracker::RetrievePositions( float *position ) {

	// Copy data into an array in which missing values are indicate by INVISIBLE.
	for ( int i = 0; i < nAcqFrames * 3; i++ ) {
			position[i] = recordedPosition[i];
	}
	return( nAcqFrames );
}

bool DexVirtualTracker::GetAcquisitionState( void ) {
	return( acquisition_on );
}


/*********************************************************************************/

void DexVirtualTracker::SetMovementType( int type ) {
	simulated_movement = type;
}

#define ATTRACTION_TIME_CONSTANT 0.5

int DexVirtualTracker::Update( void ) {

	int i = 0;
	int exit_status = 0;


	/*
	 * In the real system we measure the actual position of the manipulandum
	 *   using the call UpdateManipulandumPosition();
	 * Here we simulate the measured movement of the hand.
	 */

	double t_pos[3];
	t_pos[X] = targets->targetPosition[targets->lastTargetOn][X] + 25.0;
	t_pos[Y] = targets->targetPosition[targets->lastTargetOn][Y];
	t_pos[Z] = targets->targetPosition[targets->lastTargetOn][Z];

	switch ( simulated_movement ) {

	case TARGETED_PROTOCOL: // The manipulandum moves magically toward the most recently lit target.
	case COLLISION_PROTOCOL:

		for ( i = 0; i < 3; i++ ) {
			manipulandumPosition[i] += ATTRACTION_TIME_CONSTANT * 
				( t_pos[i] - manipulandumPosition[i] );
		}
		break;

	case OSCILLATION_PROTOCOL: // The manipulandum oscillates around the center target.

		if ( TimerElapsedTime( &oscillate_timer ) < 5.0 ) {
			for ( i = 0; i < 3; i++ ) {
				manipulandumPosition[i] += ATTRACTION_TIME_CONSTANT * 2 *
					( t_pos[i] - manipulandumPosition[i] );
			}
		}
		else if ( TimerElapsedTime( &oscillate_timer ) < 15.0 ){
			manipulandumPosition[X] = targets->targetPosition[targets->nTargets / 2][X] + 25;
			manipulandumPosition[Y] = targets->targetPosition[targets->nTargets / 2][Y] + 50.0 * sin( 2 * PI * TimerElapsedTime( &oscillate_timer ) );
			manipulandumPosition[Z] = targets->targetPosition[targets->nTargets / 2][Z];
		}
		else {
			for ( i = 0; i < 3; i++ ) {
				manipulandumPosition[i] += ATTRACTION_TIME_CONSTANT * 2 *
					( targets->targetPosition[targets->nTargets / 2][i] - manipulandumPosition[i] );
			}
		}

		break;

	}

	if ( acquisition_on ) {
		for ( int j = 0; j < 3; j++ ) recordedPosition[ nAcqFrames * 3 + j] = manipulandumPosition[j];
		if ( nAcqFrames < nFrames ) nAcqFrames++;
	}

	return( 0 );

}

#endif
