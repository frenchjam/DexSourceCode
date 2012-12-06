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
void DexTracker::GetUnitPlacement( int unit, Vector3 &pos, Quaternion &ori ) {
	MessageBox( NULL, "GetUnitPlacement() undefined.", "DexTracker", MB_OK );
}
bool DexTracker::PerformAlignment( void ) {
	return( NORMAL_EXIT );
}
void DexTracker::CopyMarkerFrame( CodaFrame &destination, CodaFrame &source ) {
	int mrk;
	destination.time = source.time;
	for ( mrk = 0; mrk < nMarkers; mrk++ ) {
		destination.marker[mrk].visibility = source.marker[mrk].visibility;
		CopyVector( destination.marker[mrk].position, source.marker[mrk].position );
	}
}

