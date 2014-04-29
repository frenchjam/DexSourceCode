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
double fov_min_x = -1500.0, fov_max_x = 1500.0;
double fov_min_y =  1500.0, fov_max_y = 4000.0;
double fov_min_z = -1500.0, fov_max_z = 1500.0;

// Paramters used to test if the tracker units are still aligned.
double	alignmentTolerance = 5.0;				// Allowable misalignment of marker positions between CODAs.
int		alignmentRequiredGood = 2;				// How many of the markers used to check the alignment must be within threshold?
												// If the alignment is off, all will be off. If the alignment is good,
												//  some might nevertheless be bad.
												// This allows for the possibility that some of the markers may not be visible,
												//  or that some are disturbed by a poor viewing angle or a reflection.

// Acquire some data to verify where the chair and frame were at the time of alignment.
double alignmentAcquisitionDuration = 5.0;		// How much data to acquire. In theory, it could be 1 sample, 
												//  but taking more allows us to assess the noise.

// The following are the specifications of the expected position and orientation of the
//  CODA units, depending on whether the alignment was done in the upright or supine configurations.
// Values are rounded versions of data taken from the DEX EM in Toulouse.

// Coda Unit #1
Vector3		expected_coda1_position_upright = { -500, 800, 2400};
Quaternion	expected_coda1_orientation_upright = {0.5, -0.5, 0.5, -0.5};

Vector3		expected_coda1_position_supine = { 500, 2600, 500};
Quaternion	expected_coda1_orientation_supine = {-0.707107, 0.0, .707107, 0.0};

// Coda Unit #2
Vector3		expected_coda2_position_upright = {     0, 1200, 2200};
Quaternion	expected_coda2_orientation_upright = {0.0, -0.707, 0.707, 0.0};

Vector3		expected_coda2_position_supine = { 0, 2600, 900};
Quaternion	expected_coda2_orientation_supine = { 1.0, 0.0, 0.0, 0.0};

// How close do we need to be to the expected position and orientation?
double codaUnitPositionTolerance = 500.0;		// Allowable displacement wrt expected position, in mm.
double codaUnitOrientationTolerance = 90.0;		// Allowable rotation wrt expected orientation, in degrees.

double codaUnitPositionRelaxed =  1000.0;		// Use this if all you really care about is upright vs. supine..
double codaUnitOrientationIgnore = 179.9;		// Use this if you don't care what the orientation is.


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

// This routine takes the operator through the steps of setting the configuration of the hardware (upright or supine)
//  and doing the tracker alignment. It is intended to be a separate script that will be run as part of a procedure.

int RunInstall( DexApparatus *apparatus, const char *params ) {

	int status = 0;

	// Check which configuration is to be used and prompt the subject to install the apparatus accordingly.
	DexSubjectPosture desired_posture = ParseForPosture( params );
	if ( desired_posture == PostureSeated ) status = apparatus->WaitSubjectReady("CalibrateSeated.bmp", "Install the target box for seated operation." );
	else status = apparatus->WaitSubjectReady("CalibrateSupine.bmp", "Install the target box for supine operation." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum in the holder on the chair.
	status = apparatus->WaitSubjectReady("ManipInChair.bmp", "Place the manipulandum in the holder as shown. Check that locker door is fully open and that manipulandum is in view." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the target bar in the right side position.
	// Actually, this is not really necessary. The origin is the marker on the box and 
	//  whether the bar is to the right or to the left, it points in the positive Y direction.
	// So we could skip this step if we wanted to.
	status = apparatus->WaitSubjectReady("BarRight.bmp", "Make sure that the target bar\nis in the right side position." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that the 4 reference markers and the manipulandum are in the ideal field-of-view of each Coda unit.
	apparatus->ShowStatus( "Checking field of view ...", "wait.bmp" );
	status = apparatus->CheckTrackerFieldOfView( 0, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
		"Markers not centered for CODA Unit #1.\n - Is the unit properly boresighted?\n - Are the box and bar markers visible?", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerFieldOfView( 1, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
			"Markers not centered for CODA Unit #2.\n - Is the unit properly boresighted?\n - Are the box and bar markers visible?", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// Perform the alignment based on those markers.
	apparatus->ShowStatus( "Performing alignment ...", "wait.bmp" );
	status = apparatus->PerformTrackerAlignment( "Error performing tracker alignment. - Target bar in the right position?\n- Reference markers in view?" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	apparatus->ShowStatus( "Checking tracker placement ...", "wait.bmp" );
	if ( desired_posture == PostureSeated ) {

		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionTolerance, 
											expected_coda1_orientation_upright, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.\n -Is setup in SEATED configuration?\n -Are coda units arranged properly?", "SetupSeated.bmp" );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionTolerance, 
											expected_coda1_orientation_supine, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.\n - Is setup in SUPINE configuration?\n - Are coda units arranged properly?", "SetupSupine.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	if ( apparatus->nCodas > 1 ) {

		if ( desired_posture == PostureSeated ) {

			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionTolerance, 
												expected_coda2_orientation_upright, codaUnitOrientationTolerance, 
												"Placement error - Coda Unit 2.\n -Is setup in SEATED configuration?\n -Are coda units arranged properly?", "SetupSeated.bmp" );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionTolerance, 
												expected_coda2_orientation_supine, codaUnitOrientationTolerance, 
												"Placement error - Coda Unit 2.\n - Is setup in SUPINE configuration?\n - Are coda units arranged properly?", "SetupSupine.bmp" );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		apparatus->ShowStatus( "Checking tracker alignment ...", "wait.bmp" );
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, "Coda misalignment detected!\n - Did a CODA unit get bumped?\n - Are the markers occluded?" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	}
	

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->ShowStatus( "Acquiring baseline data. Please wait.", "wait.bmp" );
	apparatus->StartAcquisition( "ALGN", maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition();

	apparatus->ShowStatus( "Check visibility ..." );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Maniplandum obscured from view." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	apparatus->ShowStatus( "Visibility OK." );
	apparatus->Wait( 1.0 );

	//need to change picture
	status = apparatus->WaitSubjectReady("OpenRetainer.bmp", "Deploy the retainer on the target frame." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	status = apparatus->WaitSubjectReady("RetainerManip.bmp", "Move the manipulandum up to the retainer on the target frame and close the locker door." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	if ( desired_posture == PostureSeated ) {
		status = apparatus->fWaitSubjectReady( "BeltsSeated.bmp", MsgQueryReadySeated, OkToContinue );
	}
	else if ( desired_posture == PostureSupine ) {
		status = apparatus->fWaitSubjectReady( "BeltsSupine.bmp", MsgQueryReadySupine, OkToContinue );
	}
	if ( status == ABORT_EXIT ) exit( status );


	apparatus->HideStatus();

	return( NORMAL_EXIT );
}

/************************************************************************************************************************************/

// This routine provides the steps to see if hardware is in the configuration that we expect it to be in.
// If the configuration is already good, it simply returns. If it is not as expected, it gives hints about what to do.
// This routine is meant to be run at the start of other tasks. Basically it is there to make sure that the hardware 
// is in the right configuration if the tasks are executed out of order.

int CheckInstall( DexApparatus *apparatus, DexSubjectPosture desired_posture, DexTargetBarConfiguration desired_bar_position ) {

	int status = 0;

	// Are the CODA bars where we think they should be, given the desired postural configuration?
	// Here we are mainly concerned with checking if the alignment was done in the desired configuration.
	// We don't care so much if the CODAs are swapped or installed in a different configuration.
	// So we use a relaxed constraint on the position of each unit and we ignore the orientation of the units.

	apparatus->Comment( "################################################################################" );
	apparatus->Comment( "Operations to check the hardware configuration." );

	apparatus->ShowStatus( "Checking hardware configuration ..." );
	if ( desired_posture == PostureSeated ) {

		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionRelaxed, 
											expected_coda1_orientation_upright, codaUnitOrientationIgnore, 
											"Is setup in SEATED configuration?\nWas CODA alignment performed?", "SetupSeated.bmp" );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionRelaxed, 
											expected_coda1_orientation_supine, codaUnitOrientationIgnore, 
											"Is setup in SUPINE configuration?\nWas CODA alignment performed?", "SetupSupine.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	if ( apparatus->nCodas > 1 ) {

		if ( desired_posture == PostureSeated ) {

			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionRelaxed, 
												expected_coda2_orientation_upright, codaUnitOrientationIgnore, 
												"Placement error - Coda Unit 2.\n - Is setup in SEATED configuration?\n - Was CODA alignment performed?", "SetupSeated.bmp" );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionRelaxed, 
												expected_coda2_orientation_supine, codaUnitOrientationIgnore, 
												"Placement error - Coda Unit 2.\n - Is setup in SUPINE configuration?\n - Was CODA alignment performed?", "SetupSupine.bmp" );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, 
			"Coda misalignment detected!\n - Are the markers in the line-of-sight?\n - Did a CODA unit get bumped?", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	}
		

	// The current implementation of SelectAndCheckConfiguration() does not handle doing the aligment in the supine configuration. 
	// The test to decide if the hdw is in the upright or supine configuration assumes that the alignment was done in the upright 
	//  configuration. If the alignment is done in the supine configuration, then the test will say that it is in the upright 
	//  configuration when the apparatus is in the supine configuration and vice versa.
	// Not to fear. The tests of the tracker placement above have already determined that the hardware was installed in the proper
	//  configuation and that the alignment was done in that configuration. So what we do here is set the desired configuration 
	//  to be upright (seated), which will appear to be true in both the supine and upright cases. Nevertheless, the error message
	//  and picture displayed to the subject if the test fails should be what we really want.

	// TODO: Create pictures specific to each configuration (upright/supine X bar left/bar right).
	if ( desired_posture == PostureSupine ) {
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Apparatus in SUPINE configuration?\nTarget bar in the LEFT position?\nReference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Apparatus in SUPINE configuration?\nTarget bar in the RIGHT position?\nReference markers occluded?\n", PostureSeated, desired_bar_position, DONT_CARE );
	}
	else {
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Apparatus in SEATED configuration?\n - Target bar in the LEFT position?\n - Reference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( "HdwConfD.bmp", "Apparatus in SEATED configuration?\n - Target bar in the RIGHT position?\n - Reference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
	}
	

	apparatus->HideStatus();


	return( status );
}
