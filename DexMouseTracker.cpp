/***************************************************************************/
/*                                                                         */
/*                              DexMouseTracker                            */
/*                                                                         */
/***************************************************************************/

// This is a simulated tracker that allows us to work without a CODA.
// It captures movements of the mouse and converts them into 
//  simulated movements of a set of CODA markers.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <fMessageBox.h>

#include <VectorsMixin.h>
#include "..\DexSimulatorApp\resource.h"
#include "DexTracker.h"

// DexMouseTracker will return these values when the current
// Coda transformation is requested.

Vector3		SimulatedCodaOffset[2] = {
	{  1000,    0.0, -2500.0 }, 
	{   0.0, -900.0, -2500.0 }
};

Matrix3x3	SimulatedCodaRotation[2] = {
	{{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},
	{{1.0, 0.0, 0.0},{ 0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}
};

/*********************************************************************************/

void DexMouseTracker::Initialize( void ) {

	acquisitionOn = false; 
	overrun = false;
	nAcqFrames = 0;
	nPolled = 0;

	char *filename = "DexMouseTrackerDebug.mrk";

	// Open a file to store the positions and orientations that were used by 
	// GetCurrentMarkerFrame() to simulate the marker positions. This allows us
	// to compare with the position and orientation computed by DexApparatus
	// for testing purposes.

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fMessageBox( MB_OK, "DexMouseTracker", "Error openning file for write:\n %s", filename );
		exit( -1 );
	}
	fprintf( fp, "Time\tPx\tPy\tPz\tQx\tQy\tQz\tQm\n" );

}

void DexMouseTracker::Quit( void ) {
	fclose( fp );
}

/*********************************************************************************/

void DexMouseTracker::GetUnitTransform( int unit, Vector3 &offset, Matrix3x3 &rotation ) {

	// Simulate a placement error by swapping the units;
	if ( !IsDlgButtonChecked( dlg, IDC_CODA_POSITIONED ) ) {
		unit = 1 - unit;
	}

	// Return constant values for the Coda unit placement.
	// The real tracker will compute these from the Coda transformations.
	CopyVector( offset, SimulatedCodaOffset[unit] );
	CopyMatrix( rotation, SimulatedCodaRotation[unit] );

}

/*********************************************************************************/

int DexMouseTracker::PerformAlignment ( int origin, int x_negative, int x_positive, int xy_negative, int xy_positive ) {

	// Normally one would fill a record with the marker assignments
	// for the CODA marker alignment procedure, then execute it.
	// It should return any errors reported by the CODA.

	// Unchecking the box shows that we did the alignment.
	CheckDlgButton( dlg, IDC_CODA_ALIGNED, true );
	// For now I just assume that it will be successful.
	return( NORMAL_EXIT );

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

/*********************************************************************************/

int DexMouseTracker::RetrieveMarkerFrames( CodaFrame frames[], int max_frames, int unit ) {

	int previous;
	int next;
	int frm;

	Vector3f	jump, delta;
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
		if ( time < polledMarkerFrames[previous].time ) {
			CopyMarkerFrame( frames[frm], polledMarkerFrames[previous] );
		}

		else {
			// See if we have caught up with the real-time data.
			if ( time > polledMarkerFrames[next].time ) {
				previous = next;
				// Find the next real-time frame that has a time stamp later than this one.
				while ( polledMarkerFrames[next].time <= time && next < nPolled ) next++;
				// If we reached the end of the real-time samples, then we are done.
				if ( next >= nPolled ) {
					break;
				}
			}

			// Compute the time difference between the two adjacent real-time frames.
			interval = polledMarkerFrames[next].time - polledMarkerFrames[previous].time;
			// Compute the time between the current frame and the previous real_time frame.
			offset = time - polledMarkerFrames[previous].time;
			// Use the relative time to interpolate.
			relative = offset / interval;

			for ( int j = 0; j < nMarkers; j++ ) {
				// Both the previous and next polled frame must be visible for the 
				// interpolated sample to be visible.
				frames[frm].marker[j].visibility = 
					 ( polledMarkerFrames[previous].marker[j].visibility
					   && polledMarkerFrames[next].marker[j].visibility );

				if ( frames[frm].marker[j].visibility ) {
					SubtractVectors( jump, 
										polledMarkerFrames[next].marker[j].position,
										polledMarkerFrames[previous].marker[j].position );
					ScaleVector( delta, jump, (float) relative );
					AddVectors( frames[frm].marker[j].position, polledMarkerFrames[previous].marker[j].position, delta );
				}
			}
		}
	}
	nAcqFrames = frm;
	return( nAcqFrames );

}

/*********************************************************************************/

bool DexMouseTracker::GetCurrentMarkerFrame( CodaFrame &frame ) {


	POINT mouse_position;
	GetCursorPos( &mouse_position );
	RECT rect;
	GetWindowRect( GetDesktopWindow(), &rect );

	Vector3		position, rotated;
	Quaternion	Ry, Rz, Q;

	int mrk, id;

	// Map mouse coordinates to world coordinates. The factors used here are empirical.
	float y =  (double) ( mouse_position.y - rect.top ) / (double) ( rect.bottom - rect.top ) * 230.0;
	float z =  (double) -100.0 + (mouse_position.x - rect.right) / (double) ( rect.right - rect.left ) * 305.0;
	float x;
	
	if ( IsDlgButtonChecked( dlg, IDC_CODA_WOBBLY ) ) {
		// We will make the manipulandum rotate in a strange way as a function of the distance from 0.
		// This is just so that we can test the routines that compute the manipulandum position and orientation.
		double theta = (double) (mouse_position.y - rect.top) / (double) ( rect.bottom - rect.top ) * 45.0;
		double gamma = (double) (mouse_position.x - rect.right) / (double) ( rect.right - rect.left ) * 45.0;
		SetQuaterniond( Ry, theta, iVector );
		SetQuaterniond( Rz, gamma, jVector );
		MultiplyQuaternions( Q, Ry, Rz );
		// Make the movement a little bit in X as well so that we test the routines in 3D.
		x = 0.0 + 5.0 * sin( y / 80.0);
	}
	else {
		CopyQuaternion( Q, nullQuaternion );
		x = 0.0;
	}

	position[X] = x;
	position[Y] = y;
	position[Z] = z;

	frame.time = DexTimerElapsedTime( acquisitionTimer );

	// Displace the manipulandum with the mouse and make it rotate.
	for ( mrk = 0; mrk < nManipulandumMarkers; mrk++ ) {
		id = ManipulandumMarkerID[mrk];
		RotateVector( rotated, Q, ManipulandumBody[mrk] );
		AddVectors( frame.marker[id].position, position, rotated );
		frame.marker[id].visibility = true;
	}

	// Displace the wrist with the mouse, but don't rotate it.
	for ( mrk = 0; mrk < nWristMarkers; mrk++ ) {
		id = WristMarkerID[mrk];
		AddVectors( frame.marker[id].position, position, WristBody[mrk] );
		frame.marker[id].visibility = true;
	}

	// Just set the target frame markers at their nominal fixed positions.
	for ( mrk = 0; mrk < nFrameMarkers; mrk++ ) {
		id = FrameMarkerID[mrk];
		CopyVector( frame.marker[id].position, TargetFrameBody[mrk] );
	}

	// Shift the vertical bar markers to simulate being in the left position.
	if ( IsDlgButtonChecked( dlg, IDC_LEFT ) ) {
		frame.marker[FrameMarkerID[2]].position[X] += 300.0;
		frame.marker[FrameMarkerID[3]].position[X] += 300.0;
	}

	// TODO: Transform all the marker positions if the dialog box
	// says that the apparatus is in the supine position.
	if ( IsDlgButtonChecked( dlg, IDC_SUPINE ) ) {
	}

	// Output the position and orientation used to compute the simulated
	// marker positions. This is for testing only.
	fprintf( fp, "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
		frame.time, 
		position[X], position[Y], position[Z],
		Q[X], Q[Y], Q[Z], Q[M] );

	return( true );

}

// Get the latest frame of marker data from the specified unit.
// Marker positions are expressed in the aligned reference frame.
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


/*********************************************************************************/

int DexMouseTracker::Update( void ) {

	// Store the time series of data.
	if ( DexTimerTimeout( acquisitionTimer ) ) {
		acquisitionOn = false;
		overrun =true;
	}
	if ( acquisitionOn && nPolled < DEX_MAX_MARKER_FRAMES ) {
		CodaFrame	frame;
		GetCurrentMarkerFrame( frame );
		CopyMarkerFrame( polledMarkerFrames[nPolled], frame );
		if ( nPolled < DEX_MAX_MARKER_FRAMES ) nPolled++;
	}
	return( 0 );

}