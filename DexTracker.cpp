/*********************************************************************************/
/*                                                                               */
/*                                  DexTracker.cpp                               */
/*                                                                               */
/*********************************************************************************/

// Base class for a Dexterous Manipulation Tracker.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <VectorsMixin.h>

#include "DexTracker.h"

/***************************************************************************/

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
bool  DexTracker::GetCurrentMarkerFrameUnit( CodaFrame &frame, int unit ) { 
	return( false );
}
bool  DexTracker::GetCurrentMarkerFrameIntrinsic( CodaFrame &frame, int unit ) { 
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
bool DexTracker::CheckAcquisitionOverrun( void ) {
	return( false );
}

/**************************************************************************************/

void DexTracker::GetUnitTransform( int unit, Vector3 &offset, Matrix3x3 &rotation ) {
	MessageBox( NULL, "GetUnitTransform() undefined.", "DexTracker", MB_OK );
}

void DexTracker::GetUnitPlacement( int unit, Vector3 &pos, Quaternion &ori ) {

	Vector3		offset;
	Matrix3x3	rotation;
	Matrix3x3	ortho;

	GetUnitTransform( unit, offset, rotation );

	// I have to check: Is the position equal to its offset, 
	//  or the negative of the offset?
	ScaleVector( pos, offset, -1.0 );
	// I want to be sure that the rotation matrix is really a rotation matrix.
	// This means that it must be orthonormal.
	// In fact, I am not convinced that CODA guarantees an orthonormal matrix.
	OrthonormalizeMatrix( ortho, rotation );
	// Express the orientation of the CODA unit as a quaternion.
	MatrixToQuaternion( ori, ortho );

}


/**************************************************************************************/

int DexTracker::PerformAlignment( int origin, int x_negative, int x_positive, int xy_negative, int xy_positive ) {
	return( NORMAL_EXIT );
}

/**************************************************************************************/

void DexTracker::CopyMarkerFrame( CodaFrame &destination, CodaFrame &source ) {
	int mrk;
	destination.time = source.time;
	for ( mrk = 0; mrk < nMarkers; mrk++ ) {
		destination.marker[mrk].visibility = source.marker[mrk].visibility;
		CopyVector( destination.marker[mrk].position, source.marker[mrk].position );
	}
}

