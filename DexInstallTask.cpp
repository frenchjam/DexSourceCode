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
double fov_min_y =  2000.0, fov_max_y = 4000.0;
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
unsigned long alignmentMarkerMask = 0x03C00000;

// A bit mask describing which markers are used to perform the field-of-view check.
// This includes the manipulandum and the reference frame markers, with the 
//  assumption that the manipulandum is placed on the back of the chair in a 
//  visible position during the alignment procedure.
unsigned long fovMarkerMask = 0x03C00000;

/*********************************************************************************/

int RunInstall( DexApparatus *apparatus, const char *params ) {

	int status;

	Quaternion expected_orientation[2];

	// Where do we expect the CODAs to be with respect to the reference frame?.
	Vector3 expected_position[2] = {{-1000.0, 0.0, 2500.0}, {0.0, 900.0, 2500.0}};
	// Express the expected orientation of each fo the CODA units as a quaternion.
	apparatus->SetQuaterniond( expected_orientation[0], 90.0, apparatus->kVector );
	apparatus->SetQuaterniond( expected_orientation[1],  0.0, apparatus->iVector );


	// Check that the 4 reference markers are in the ideal field-of-view of each Coda unit.
	ShowStatus( "Checking field of view ..." );
	status = apparatus->CheckTrackerFieldOfView( 0, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerFieldOfView( 1, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// Perform the alignment based on those markers.
	ShowStatus( "Performing alignment ..." );
	status = apparatus->PerformTrackerAlignment();
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	ShowStatus( "Check tracker placement ..." );
	status = apparatus->CheckTrackerPlacement( 0, 
										expected_position[0], codaUnitPositionTolerance, 
										expected_orientation[0], codaUnitOrientationTolerance, 
										"Placement error - Coda Unit 0." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerPlacement( 1, 
											expected_position[1], codaUnitPositionTolerance, 
											expected_orientation[1], codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1." );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// Check that the tracker is still aligned.
	ShowStatus( "Check tracker alignment ..." );
	status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum on the chair.
	status = apparatus->WaitSubjectReady( "Place maniplandum in specified position on the chair." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform a short acquisition to measure where the manipulandum is.
	ShowStatus( "Acquire data ..." );
	apparatus->StartAcquisition( maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition();
	apparatus->SaveAcquisition( "ALGN" );

	ShowStatus( "Check visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	return( NORMAL_EXIT );
}
