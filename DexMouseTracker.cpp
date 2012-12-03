/***************************************************************************/
/*                                                                         */
/*                              DexMouseTracker                            */
/*                                                                         */
/***************************************************************************/

// This is a simulated tracker that allows us to work without a CODA.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "DexTracker.h"
#include "..\DexSimulatorApp\resource.h"

/*********************************************************************************/

void DexMouseTracker::Initialize( void ) {
	acquisitionOn = false; 
	overrun = false;
	nAcqFrames = 0;
	nPolled = 0;
}

/*********************************************************************************/

void DexMouseTracker::GetUnitPlacement( int unit, Vector3 &pos, Quaternion &ori ) {

	Quaternion q1, q2;

	// Simulate a placement error by swapping the units;
	if ( !IsDlgButtonChecked( dlg, IDC_CODA_POSITIONED ) ) {
		unit = 1 - unit;
	}

	// Return constant values for the Coda unit placement.
	// The real tracker will compute these from the Coda transformations.
	if ( unit == 0 ) {
		pos[X] = -1000.0;
		pos[Y] = 0.0;
		pos[Z] = 2500.0;
		SetQuaterniond( q1, 90.0, kVector );
		SetQuaterniond( q2, 30.0, jVector );
		MultiplyQuaternions( ori, q2, q1 );
	}
	else {
		pos[X] = 0.0;
		pos[Y] = 900.0;
		pos[Z] = 2500.0;
		SetQuaterniond( ori, 45.0, iVector );
	}

}

/*********************************************************************************/

void DexMouseTracker::DoAlignment( const char *msg ) {
	CheckDlgButton( dlg, IDC_CODA_ALIGNED, true );
}

/*********************************************************************************/

void DexMouseTracker::StartAcquisition( float max_duration ) { 
	acquisitionOn = true; 
	overrun = false;
	nPolled = 0;
	DexTimerSet( acquisitionTimer, max_duration );
	Update();
}

void DexMouseTracker::StopAcquisition( void ) {
	acquisitionOn = false;
	duration = DexTimerElapsedTime( acquisitionTimer );
}

bool DexMouseTracker::GetAcquisitionState( void ) {
	return( acquisitionOn );
}

bool DexMouseTracker::CheckOverrun( void ) {
	return( overrun );
}

int DexMouseTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames ) {

	int previous;
	int next;
	int frm;

	Vector3	jump, delta;
	double	time, interval, offset, relative;

	// Copy data into an array.
	previous = 0;
	next = 1;

	// Fill an array of frames at a constant frequency by interpolating the
	// frames that were taken at a variable frequency in real time.
	for ( frm = 0; frm < max_frames; frm++ ) {

		// Compute the time of each slice at a constant sampling frequency.
		time = (double) frm * samplePeriod;
		frames[frm].time = (float) time;

		// Fill the first frames with the same position as the first polled frame;
		if ( time < recordedMarkerFrames[previous].time ) {
			CopyMarkerFrame( frames[frm], recordedMarkerFrames[previous] );
		}

		else {
			// See if we have caught up with the real-time data.
			if ( time > recordedMarkerFrames[next].time ) {
				previous = next;
				// Find the next real-time frame that has a time stamp later than this one.
				while ( recordedMarkerFrames[next].time <= time && next < nPolled ) next++;
				// If we reached the end of the real-time samples, then we are done.
				if ( next >= nPolled ) {
					break;
				}
			}

			// Compute the time difference between the two adjacent real-time frames.
			interval = recordedMarkerFrames[next].time - recordedMarkerFrames[previous].time;
			// Compute the time between the current frame and the previous real_time frame.
			offset = time - recordedMarkerFrames[previous].time;
			if ( offset == 0.0 ) {
				offset = offset;
			}
			// Use the relative time to interpolate.
			relative = offset / interval;

			for ( int j = 0; j < nMarkers; j++ ) {
				// Both the previous and next polled frame must be visible for the 
				// interpolated sample to be visible.
				frames[frm].marker[j].visibility = 
					 ( recordedMarkerFrames[previous].marker[j].visibility
					   && recordedMarkerFrames[next].marker[j].visibility );

				if ( frames[frm].marker[j].visibility ) {
					SubtractVectors( jump, 
										recordedMarkerFrames[next].marker[j].position,
										recordedMarkerFrames[previous].marker[j].position );
					ScaleVector( delta, jump, (float) relative );
					AddVectors( frames[frm].marker[j].position, recordedMarkerFrames[previous].marker[j].position, delta );
				}
			}
		}
	}
	nAcqFrames = frm;
	return( nAcqFrames );

}

/*********************************************************************************/

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

bool DexMouseTracker::GetCurrentMarkerFrameUnit( CodaFrame &frame, int unit ) {

	GetCurrentMarkerFrame( frame );

	// Simulate mis-alignment if the aligned box is not checked.
	if( !IsDlgButtonChecked( dlg, IDC_CODA_ALIGNED ) ) {
		if ( unit == 0 ) {
			for ( int mrk = 0; mrk < nMarkers; mrk++ ) frame.marker[mrk].position[X] += 10.0;
		}
		else {
			for ( int mrk = 0; mrk < nMarkers; mrk++ ) frame.marker[mrk].position[X] -= 10.0;
		}
	}
	return( true );
}

bool DexMouseTracker::GetCurrentMarkerFrameIntrinsic( CodaFrame &frame, int unit ) {

	CodaFrame	iframe;
	int status;

	// Retrieve the offset and rotation matrix from the Coda for this unit.
	// Here I am setting a null transformation.
	Vector3 offset = { 0.0, 0.0, 0.0 }, delta;
	// Will need to take the inverse (transpose?) of the rotation matrix.
	Matrix3x3 rotation = {{1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{0.0, 0.0, 1.0}};

	status = GetCurrentMarkerFrameUnit( frame, unit );
	if ( !status ) return( false );

	for ( int mrk = 0; mrk < nMarkers; mrk++ ) {
		frame.marker[mrk].visibility = iframe.marker[mrk].visibility;
		if ( iframe.marker[mrk].visibility ) {
			SubtractVectors( delta, iframe.marker[mrk].position, offset );
			MultiplyVector( frame.marker[mrk].position, rotation, delta );
		}
	}

	return( true );
}

/*********************************************************************************/

int DexMouseTracker::Update( void ) {

	POINT mouse_position;
	GetCursorPos( &mouse_position );
	RECT rect;
	GetWindowRect( GetDesktopWindow(), &rect );

	float z =  (double) -100 + (mouse_position.x - rect.right) / (double) ( rect.right - rect.left ) * 240.0;
	float y =  (double)  mouse_position.y / (double) ( rect.bottom - rect.top ) * 240.0;

	// Marker 0 follows the mouse.
	currentMarkerFrame.marker[0].position[Z] = z;
	currentMarkerFrame.marker[0].position[Y] = y;
	currentMarkerFrame.marker[0].position[X] = 60.0;
	currentMarkerFrame.marker[0].visibility = true;

	// Other markers follow a sinusoid.
	for ( int j = 1; j < nMarkers; j++ ) {
		currentMarkerFrame.marker[j].visibility = true;
		for ( int k = 0; k < 3; k++ ) {
			currentMarkerFrame.marker[j].position[k] = (float) cos( DexTimerElapsedTime( acquisitionTimer ) );
		}
	}

	// Store the time series of data.
	if ( DexTimerTimeout( acquisitionTimer ) ) {
		acquisitionOn = false;
		overrun =true;
	}
	if ( acquisitionOn && nPolled < DEX_MAX_MARKER_FRAMES ) {
		recordedMarkerFrames[ nPolled ].time = (float) DexTimerElapsedTime( acquisitionTimer );
		for ( int j = 0; j < nMarkers; j++ ) {
			recordedMarkerFrames[nPolled].marker[j].visibility = currentMarkerFrame.marker[j].visibility;
			for ( int k = 0; k < 3; k++ ) {
				recordedMarkerFrames[nPolled].marker[j].position[k] = currentMarkerFrame.marker[j].position[k]; 
			}
		}
		if ( nPolled < DEX_MAX_MARKER_FRAMES ) nPolled++;
	}
	return( 0 );

}