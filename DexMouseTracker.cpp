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
#include "DexSimulatorGUI.h"
#include "DexTracker.h"

// DexMouseTracker will return these values when the current
// Coda transformation is requested.

// Values returned when the alignment is assumed to have been done in the upright configuration.
// These are taken from a real DEX file, rounded to 90° rotations. 

Vector3		SimulatedUprightCodaOffset[2] = {
	{  -600.0,  700.0,  2200.0 }, 
	{     0.0, 1200.0,  2000.0 }
};

Matrix3x3	SimulatedUprightCodaRotation[2] = {
	{{ 0.0, -1.0, 0.0},{ 0.0,  0.0, -1.0}, {1.0,  0.0, 0.0}},
	{{-1.0,  0.0, 0.0},{ 0.0,  0.0, -1.0}, {0.0, -1.0, 0.0}}
};

// Values returned when the alignment is assumed to have been done in the supine configuration.
Vector3		SimulatedSupineCodaOffset[2] = {
	{  550.0, 2600.0,    400.0 }, 
	{    0.0, 2500.0,    900.0 }
};

Matrix3x3	SimulatedSupineCodaRotation[2] = {
	{{0.0, 0.0, -1.0},{0.0, -1.0, 0.0}, {-1.0, 0.0, 0.0}},
	{{1.0, 0.0, 0.0},{ 0.0, -1.0, 0.0}, {0.0, 0.0, -1.0}}
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
	if ( TRACKER_ALIGNED_SUPINE == SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0 ) ) {
		CopyVector( offset, SimulatedSupineCodaOffset[unit] );
		CopyMatrix( rotation, SimulatedSupineCodaRotation[unit] );
	}
	else {
		CopyVector( offset, SimulatedUprightCodaOffset[unit] );
		CopyMatrix( rotation, SimulatedUprightCodaRotation[unit] );
	}


}

/*********************************************************************************/

int DexMouseTracker::PerformAlignment ( int origin, int x_negative, int x_positive, int xy_negative, int xy_positive ) {

	// Normally one would fill a record with the marker assignments
	// for the CODA marker alignment procedure, then execute it.
	// It should return any errors reported by the CODA.

	// Simulate an error if the reference markers are not visible.
	if ( IsDlgButtonChecked( dlg, IDC_BAR_OCCLUDED ) || IsDlgButtonChecked( dlg, IDC_BOX_OCCLUDED ) ) {
		SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_SETCURSEL, TRACKER_UNALIGNED, 0 );
		return( ERROR_EXIT );
	}

	// Remember what is the configuration when the alignment is performed.
	if ( IsDlgButtonChecked( dlg, IDC_SUPINE ) ) SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_SETCURSEL, TRACKER_ALIGNED_SUPINE, 0 );
	else if ( IsDlgButtonChecked( dlg, IDC_SEATED ) ) SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_SETCURSEL, TRACKER_ALIGNED_UPRIGHT, 0 );
	else SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_SETCURSEL, TRACKER_UNALIGNED, 0 );

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
	Vector3	x_dir, y_span, z_span;
	Vector3		x_displacement, y_displacement, z_displacement;

	double		x, y, z;

	Quaternion	Ry, Rz, Q, nominalQ, midQ;
	Matrix3x3	xform = {{-1.0, 0.0, 0.0},{0.0, 0.0, 1.0},{0.0, 1.0, 0.0}};

	int mrk, id;
	
	// Just set the target frame markers at their nominal fixed positions.
	for ( mrk = 0; mrk < nFrameMarkers; mrk++ ) {
		id = FrameMarkerID[mrk];
		CopyVector( frame.marker[id].position, TargetFrameBody[mrk] );
	}

	// Shift the vertical bar markers to simulate being in the left position.
	if ( IsDlgButtonChecked( dlg, IDC_LEFT ) ) {
		frame.marker[DEX_NEGATIVE_BAR_MARKER].position[X] += 300.0;
		frame.marker[DEX_POSITIVE_BAR_MARKER].position[X] += 300.0;
	}

	// Transform the marker positions of the target box and frame according 
	//  to whether the system was upright or supine when it was aligned and
	//  according to whether the system is currently installed in the upright
	//  or supine configuration.
	if ( ( IsDlgButtonChecked( dlg, IDC_SUPINE ) && TRACKER_ALIGNED_SUPINE  != SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0 ) ) ||
		 ( IsDlgButtonChecked( dlg, IDC_SEATED ) && TRACKER_ALIGNED_UPRIGHT != SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0 ) ) )	{	
		 for ( mrk = 0; mrk < nFrameMarkers; mrk++ ) {

			id = FrameMarkerID[mrk];
			position[X] = - frame.marker[id].position[X];
			position[Z] =   frame.marker[id].position[Y];
			position[Y] =   frame.marker[id].position[Z];

			CopyVector( frame.marker[id].position, position );
		}
		MatrixToQuaternion( nominalQ, xform );
	}
	else CopyQuaternion( nominalQ, nullQuaternion );

	// Now set the visibility flag as a funciton of the GUI.
	if ( IsDlgButtonChecked( dlg, IDC_BAR_OCCLUDED ) ) {
		frame.marker[DEX_NEGATIVE_BAR_MARKER].visibility = false;
		frame.marker[DEX_POSITIVE_BAR_MARKER].visibility = false;
	}
	else {
		frame.marker[DEX_NEGATIVE_BAR_MARKER].visibility = true;
		frame.marker[DEX_POSITIVE_BAR_MARKER].visibility = true;
	}
	if ( IsDlgButtonChecked( dlg, IDC_BOX_OCCLUDED ) ) {
		frame.marker[DEX_NEGATIVE_BOX_MARKER].visibility = false;
		frame.marker[DEX_POSITIVE_BOX_MARKER].visibility = false;
	}
	else {
		frame.marker[DEX_NEGATIVE_BOX_MARKER].visibility = true;
		frame.marker[DEX_POSITIVE_BOX_MARKER].visibility = true;
	}

	// Map mouse coordinates to world coordinates. The factors used here are empirical.
	
	y =  (double) ( mouse_position.y - rect.top ) / (double) ( rect.bottom - rect.top );
	z =  (double) (mouse_position.x - rect.right) / (double) ( rect.left - rect.right );

	if ( IsDlgButtonChecked( dlg, IDC_CODA_WOBBLY ) ) {
		// We will make the manipulandum rotate in a strange way as a function of the distance from 0.
		// This is just so that we can test the routines that compute the manipulandum position and orientation.
		double theta = y * 45.0;
		double gamma = - z * 45.0;
		SetQuaterniond( Ry, theta, iVector );
		SetQuaterniond( Rz, gamma, jVector );
		MultiplyQuaternions( midQ, Rz, nominalQ );
		MultiplyQuaternions( Q, Ry, midQ );
		// Make the movement a little bit in X as well so that we test the routines in 3D.
		x = 0.0 + 5.0 * sin( y / 80.0);
	}
	else {
		CopyQuaternion( Q, nominalQ );
		x = 0.0;
	}


	// Map screen position of the mouse pointer to 3D position of the wrist and manipulandum.
	// Top of the screen corresponds to the bottom of the bar and vice versa. It's inverted to protect the right hand rule.
	// Right of the screen correponds to the nearest horizontal target and left corresponds to the farthest.
	// The X position is set to be just to the right of the target bar, wherever it is.

	SubtractVectors( y_span, frame.marker[DEX_POSITIVE_BAR_MARKER].position, frame.marker[DEX_NEGATIVE_BAR_MARKER].position );
	SubtractVectors( x_dir, frame.marker[DEX_POSITIVE_BOX_MARKER].position, frame.marker[DEX_NEGATIVE_BOX_MARKER].position );
	NormalizeVector( x_dir );
	ComputeCrossProduct( z_span, x_dir, y_span );
	 
	ScaleVector( y_displacement, y_span, y );
	ScaleVector( z_displacement, z_span, z );
	ScaleVector( x_displacement, x_dir, x );

	AddVectors( position, frame.marker[DEX_NEGATIVE_BAR_MARKER].position, x_displacement );
	AddVectors( position, position, y_displacement );
	AddVectors( position, position, z_displacement );

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
	if( TRACKER_UNALIGNED == SendDlgItemMessage( dlg, IDC_ALIGNMENT, CB_GETCURSEL, 0, 0 ) ) {
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