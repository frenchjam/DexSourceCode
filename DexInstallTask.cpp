/*********************************************************************************/
/*                                                                               */
/*                              DexInstallTask.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs the operations to install and align CODAs.
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

// Installation parameters.

// Note: The parameters used to describe the expected position and orientation are
//  set at runtime in order to take advantage of the routines that are available 
//  to initialize quaternions.

// To be sure to see the markers throughout the trial the CODAs must be at a
//  certain distance and orientation. These parameters determine the volume within
//  which we expect to see the referencce markers.
// Remember, these are in the intrinsic reference frame of the CODA unit, where
//  X is along the length of the bar, Y is the distance from the bar and Z is 'above' the bar.
double fov_min_x = -1000.0, fov_max_x = 1000.0;
double fov_min_y =  1500.0, fov_max_y = 4000.0;
double fov_min_z = -1000.0, fov_max_z = 1000.0;

double codaUnitOrientationTolerance = 30.0;		// Allowable rotation wrt expected orientation, in degrees.
double codaUnitPositionTolerance = 500.0;		// Allowable displacement wrt expected position, in mm.
double	alignmentTolerance = 5.0;				// Allowable misalignment of marker positions between CODAs.
int		alignmentRequiredGood = 2;				// How many of the markers used to check the alignment must be within threshold?
												// If the alignment is off, all will be off. If the alignment is good,
												//  some might nevertheless be bad.
												// This allows for the possibility that some of the markers may not be visible,
												//  or that some are disturbed by a poor viewing angle or a reflection.
double alignmentAcquisitionDuration = 5.0;		// Acquire some data to verify where the chair and frame were at the time of alignment.

/*********************************************************************************/

// A bit mask describing which markers are used to perform the alignment check.
// This should be set to correspond to the 4 markers on the reference frame.
unsigned long alignmentMarkerMask = 0x00000f00;

// A bit mask describing which markers are used to perform the field-of-view check.
// This should be set to correspond to the 4 markers on the reference frame.
unsigned long fovMarkerMask = 0x00000f00;

/*********************************************************************************/

int RunInstall( DexApparatus *apparatus, const char *params ) {

	int status;

	Quaternion expected_orientation[2];
    Quaternion transition_orientation[2];


	// Prompt the subject to place the manipulandum on the chair.
	status = apparatus->WaitSubjectReady( "Place the target bar in the left side position." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	// Prompt the subject to place the manipulandum on the chair.
	status = apparatus->WaitSubjectReady( "Place maniplandum in specified position on the chair." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	// Check that the 4 reference markers and the manipulandum are in the ideal field-of-view of each Coda unit.
	apparatus->ShowStatus( "Checking field of view ..." );
	status = apparatus->CheckTrackerFieldOfView( 0, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerFieldOfView( 1, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// Perform the alignment based on those markers.
	apparatus->ShowStatus( "Performing alignment ..." );
	status = apparatus->PerformTrackerAlignment();
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	apparatus->ShowStatus( "Check tracker placement ..." );

	// Where do we expect the CODAs to be with respect to the reference frame?.
	Vector3 expected_position[2] = { {-622.0, 267.0, 2382.0}, {-178.0, 828.0, 2168.0} };

	// At what orientation? Need to make 2 quaternion rotations for the vertical Coda bar.
	apparatus->SetQuaterniond( transition_orientation[0], -90.0, apparatus->kVector );
	apparatus->SetQuaterniond( transition_orientation[1], -90.0, apparatus->iVector);
	apparatus->MultiplyQuaternions(expected_orientation[0],transition_orientation[0],transition_orientation[1]);
	status = apparatus->CheckTrackerPlacement( 0, 
										expected_position[0], codaUnitPositionTolerance, 
										expected_orientation[0], codaUnitOrientationTolerance, 
										"Placement error - Coda Unit 1." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	if ( apparatus->nCodas > 1 ) {

		// This is for the 'horizontal' coda. I don't know if this is right.

		// Flip it 180° in the horizontal plane (i.e. around the Y axis).
		apparatus->SetQuaterniond( transition_orientation[1],  180.0, apparatus->jVector );
		// Tilt from pointing up to pointing slightly down.
		apparatus->SetQuaterniond( transition_orientation[0],   -115.0, apparatus->iVector);
		apparatus->MultiplyQuaternions(expected_orientation[1],transition_orientation[0], transition_orientation[1]);
	
		status = apparatus->CheckTrackerPlacement( 1, 
											expected_position[1], codaUnitPositionTolerance, 
											expected_orientation[1], codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 2." );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		apparatus->ShowStatus( "Check tracker alignment ..." );
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, "Coda misaligned!" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	}
	

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->ShowStatus( "Acquire data ..." );
	apparatus->StartAcquisition( "ALGN", maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition();

	apparatus->ShowStatus( "Check visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum obscured from view." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	apparatus->HideStatus();

	return( NORMAL_EXIT );
}
