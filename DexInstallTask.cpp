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
// This should be set to correspond to the 4 markers on the reference frame. (Marker 8 9 10 11)
unsigned long alignmentMarkerMask = 0x00000f00;

// A bit mask describing which markers are used to perform the field-of-view check.
// This should be set to correspond to the 4 markers on the reference frame.
// We could add the manipulandum markers, thus making sure that the manipulandum, which is
// presumably in the holder on the chair, is also visible.
unsigned long fovMarkerMask = 0x00000f00;

/*********************************************************************************/

int RunInstall( DexApparatus *apparatus, const char *params ) {

	int status;
    Quaternion transition_orientation[3];

	// Check which configuration is to be used and prompt the subject to install the apparatus accordingly.
	DexSubjectPosture desired_posture = ParseForPosture( params );
	if ( desired_posture == PostureSeated ) status = apparatus->WaitSubjectReady("CalibrateSeated.bmp", "Install the target box for upright (seated) operation.\nPlace the manipulandum in the holder\ninside the chair locker as shown." );
	else status = apparatus->WaitSubjectReady("CalibrateSupine.bmp", "Install the target box for supine (lying down) operation.\nPlace the manipulandum in the holder\ninside the chair locker as shown." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the target bar in the right side position.
	status = apparatus->WaitSubjectReady("BarRight.bmp", "Make sure that the target bar\nis in the right side position." );
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
	Vector3 expected_position;
	Quaternion expected_orientation;


	//	Vector3 expected_position[2] = { {-600.0, 300.0, 2000.0}, {-100.0, 800.0, 2000.0} };

	if ( desired_posture == PostureSeated ) {

		// Coda Unit #1 (referenced as unit 0 here) is located to the right of the subject (- X) in a vertical orientation.
		expected_position[X] = -600;
		expected_position[Y] =  300;
		expected_position[Z] = 2500;

		// Need to make 2 quaternion rotations for the vertical Coda bar.
		apparatus->SetQuaterniond( transition_orientation[0],  90.0, apparatus->iVector );
		apparatus->SetQuaterniond( transition_orientation[1],  90.0, apparatus->jVector);
		apparatus->MultiplyQuaternions(expected_orientation,transition_orientation[0],transition_orientation[1]);
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_position, codaUnitPositionTolerance, 
											expected_orientation, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.-Is setup in SEATED configuration?-Are coda units arranged properly?", "SetupSeated.bmp" );
	}
	else {

		// If the calibration was performed in the supine configuration, Coda Unit #1 is to the left (+X) and distance is Y..
		expected_position[X] =  300;
		expected_position[Y] = 2500;
		expected_position[Z] =  300;

		// Need to make 2 quaternion rotations for the vertical Coda bar.
		// *** I DON'T KNOW THE ANSWER!!!! ***  Need to do test and correct this.
		apparatus->SetQuaterniond( transition_orientation[0], 180.0, apparatus->iVector );
		apparatus->SetQuaterniond( transition_orientation[1],  90.0, apparatus->jVector);
		apparatus->MultiplyQuaternions(expected_orientation,transition_orientation[0],transition_orientation[1]);
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_position, codaUnitPositionTolerance, 
											expected_orientation, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.\n- Is setup in SUPINE configuration?\n - Are coda units arranged properly?", "SetupSupine.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

#if 0
	if ( apparatus->nCodas > 1 ) {

		// This is for the 'horizontal' coda. I don't know if this is right.

		// Flip it 180° in the horizontal plane (i.e. around the Y axis).
		apparatus->SetQuaterniond( transition_orientation[1],  180.0, apparatus->kVector );
		// Tilt from pointing up to pointing slightly down.
		apparatus->SetQuaterniond( transition_orientation[0],   90.0, apparatus->iVector);
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
	
#endif

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->ShowStatus( "Acquire data ..." );
	apparatus->StartAcquisition( "ALGN", maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition();

	apparatus->ShowStatus( "Check visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum obscured from view." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	apparatus->ShowStatus( "Visibility OK." );
	apparatus->Wait( 1.0 );

	status = apparatus->WaitSubjectReady("RetainerManip.bmp", "Move the manipulandum to the target frame holder." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	apparatus->HideStatus();

	return( NORMAL_EXIT );
}
