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
#include <string.h>

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
double codaUnitPositionTolerance = 900.0;		// Allowable displacement wrt expected position, in mm.
double codaUnitOrientationTolerance = 90.0;		// Allowable rotation wrt expected orientation, in degrees.

double codaUnitPositionRelaxed =  1000.0;		// Use this if all you really care about is upright vs. supine..
double codaUnitOrientationIgnore = 179.9;		// Use this if you don't care what the orientation is.

double audioCheckDuration = 10.0;

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

	fprintf( stderr, "     RunInstall: %s\n", params );

	// Check which configuration is to be used and prompt the subject to install the apparatus accordingly.
	DexSubjectPosture desired_posture = ParseForPosture( params );
	if ( desired_posture == PostureSeated ) status = apparatus->WaitSubjectReady("CalibrateSeated.bmp", "Install the target box for SEATED operation." );
	else status = apparatus->WaitSubjectReady("CalibrateSupine.bmp", "Install the target box for SUPINE operation." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum in the holder on the chair.
	status = apparatus->WaitSubjectReady("ManipInChair.bmp", "Place the manipulandum in the holder as shown. Check that locker door is fully open and that the 'Visible' indicator is green." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the target bar in the right side position.
	// Actually, this is not really necessary. The origin is the marker on the box and 
	//  whether the bar is to the right or to the left, it points in the positive Y direction.
	// So we could skip this step if we wanted to.
	status = apparatus->WaitSubjectReady("BarLeft.bmp", "Make sure that the target bar\nis in the LEFT side position." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that the 4 reference markers and the manipulandum are in the ideal field-of-view of each Coda unit.
	apparatus->ShowStatus( "Checking field of view ...", "wait.bmp" );
	status = apparatus->CheckTrackerFieldOfView( 0, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
		"Markers not centered for CODA Unit #1.\n- Is the unit properly boresighted?\n- Are the box and bar markers visible?", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerFieldOfView( 1, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
			"Markers not centered for CODA Unit #2.\n- Is the unit properly boresighted?\n- Are the box and bar markers visible?", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// Perform the alignment based on those markers.
	apparatus->ShowStatus( "Performing alignment ...", "wait.bmp" );
	status = apparatus->PerformTrackerAlignment( "Error performing tracker alignment.\n-Target bar in the LEFT position?\n- Reference markers in view?", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	apparatus->ShowStatus( "Checking tracker placement ...", "wait.bmp" );
	if ( desired_posture == PostureSeated ) {

		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionTolerance, 
											expected_coda1_orientation_upright, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.\n- Is setup in SEATED configuration?\n- Are coda units arranged properly?", "CalibrateSeated.bmp" );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionTolerance, 
											expected_coda1_orientation_supine, codaUnitOrientationTolerance, 
											"Placement error - Coda Unit 1.\n- Is setup in SUPINE configuration?\n- Are coda units arranged properly?", "CalibrateSupine.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	if ( apparatus->nCodas > 1 ) {

		if ( desired_posture == PostureSeated ) {

			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionTolerance, 
												expected_coda2_orientation_upright, codaUnitOrientationTolerance, 
												"Placement error - Coda Unit 2.\n- Is setup in SEATED configuration?\n- Are coda units arranged properly?", "CalibrateSeated.bmp" );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionTolerance, 
												expected_coda2_orientation_supine, codaUnitOrientationTolerance, 
												"Placement error - Coda Unit 2.\n- Is setup in SUPINE configuration?\n- Are coda units arranged properly?", "CalibrateSupine.bmp" );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		apparatus->ShowStatus( "Checking tracker alignment ...", "wait.bmp" );
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, "Coda misalignment detected!\n- Are the markers occluded?\n- Did a CODA unit get bumped?\n(Consult ground.)", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	}
	
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else {
		strcpy( tag, "GripCfg" );
		if ( desired_posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
		else strcat( tag, "S" ); // S is for supine.
	}

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->ShowStatus( "Acquiring baseline data. Please wait.", "wait.bmp" );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition( "Error during file save." );

	apparatus->ShowStatus( "Check visibility ...", "wait.bmp" );
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, "Manipulandum obscured from view.", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	apparatus->ShowStatus( "Visibility OK.", "ok.bmp" );
	apparatus->Wait( 1.0 );

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

	apparatus->ShowStatus( "Checking hardware configuration ...", "wait.bmp" );
	if ( desired_posture == PostureSeated ) {

		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionRelaxed, 
											expected_coda1_orientation_upright, codaUnitOrientationIgnore, 
											"Unexpected Configuration\n- Configured for Seated?\n- Was CODA alignment performed?\n(Consult with COL-CC.)", "CalibrateSeated.bmp" );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionRelaxed, 
											expected_coda1_orientation_supine, codaUnitOrientationIgnore, 
											"Unexpected Configuration\n- Configured for SUPINE?\n- Was CODA alignment performed?\n(Consult with COL-CC.)", "CalibrateSupine.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	if ( apparatus->nCodas > 1 ) {

		if ( desired_posture == PostureSeated ) {

			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionRelaxed, 
												expected_coda2_orientation_upright, codaUnitOrientationIgnore, 
												"Placement error - Coda Unit 2.\n- Is setup in SEATED configuration?\n- Was CODA alignment performed?\n(Consult with COL-CC.)", "CalibrateSeated.bmp" );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionRelaxed, 
												expected_coda2_orientation_supine, codaUnitOrientationIgnore, 
												"Placement error - Coda Unit 2.\n- Is setup in SUPINE configuration?\n- Was CODA alignment performed?\n(Consult with COL-CC.)", "CalibrateSupine.bmp" );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, 
			"Coda misalignment detected!\n- Are any markers occluded?\nIf yes, correct and <Retry>.\nIf not, consult COL-CC.", "alert.bmp" );
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
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Unexpected Configuration\n- Configured for SUPINE ?\n- Target bar on the LEFT-hand side?\nReference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Unexpected Configuration\n- Configured for SUPINE?\n- Target bar on the RIGHT-hand side?\n- Reference markers occluded?\n", PostureSeated, desired_bar_position, DONT_CARE );
	}
	else {
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( "HdwConfA.bmp", "Unexpected Configuration\n- Configured for SEATED?\n- Target bar on the LEFT-hand side?\n- Reference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( "HdwConfD.bmp", "Unexpected Configuration\n- Configured for SEATED ?\n- Target bar on the RIGHT-hand side?\n- Reference markers occluded?", PostureSeated, desired_bar_position, DONT_CARE );
	}
	

	apparatus->HideStatus();


	return( status );
}

/************************************************************************************************************************************/

// Instruct the subject to adjust the audio volume so they are sure to hear the tones.

int CheckAudio( DexApparatus *apparatus, const char *params ) {

	int status;

	status = apparatus->WaitSubjectReady("headphones.bmp", "Don the headphones and connect to the GRIP hardware. Press <OK> to continue." );
	if ( status == ABORT_EXIT ) return( status );

	char msg[1024];
	sprintf( msg, "A series of beeps is being played for %.0f seconds. Adjust the volume until you hear them clearly.", audioCheckDuration );
	apparatus->ShowStatus( msg, "volume.bmp" );
	for ( int i = 0; i < audioCheckDuration; i++ ) {
		apparatus->Beep();
		apparatus->Wait( 0.9 );
	}

	status = apparatus->WaitSubjectReady("info.bmp", "Test complete. If you did not hear the beeps, check the hardware and then repeat this task." );
	if ( status == ABORT_EXIT ) return( status );


	return( NORMAL_EXIT );

}

/************************************************************************************************************************************/

int MiscInstall ( DexApparatus *apparatus, const char *params ) {

	int status = NORMAL_EXIT;

	// Place this at the beginning of a protocol to verify the subject ID.
	if( char *ptr = strstr( params, "-checkID"  ) ) {
		char id[256];
		int i = 0;
		ptr = ptr + strlen( "-checkID" ) + 1;
		if ( *ptr == '"' ) {
			ptr++;
			while ( (*ptr != '"') && *ptr && (i < sizeof( id ) - 1) ) id[i++] = *ptr++;
			id[i] = 0;
		}
		else {
			while ( *ptr != ' ' && *ptr != '\t' && *ptr && (i < sizeof( id ) - 1) ) id[i++] = *ptr++;
			id[i] = 0;
		}

		status = apparatus->fWaitSubjectReady("confirm.bmp", "Confirm Subject ID: %s\n\nIf not correct, press <OK>, then <Back>, then <Logout>.", id );
		if ( status != NORMAL_EXIT ) return( status );
	}

	// Place this at the end of a protocol to let the subject know that he or she is finished.
	if( strstr( params, "-finish"  ) ) {
		status = apparatus->WaitSubjectReady("ok.bmp", "Protocol Terminated.\nPress <OK> | <Back> | <Logout>." );
		if ( status != NORMAL_EXIT ) return( status );
	}

	////// Hardware Tests ////////

	// Check Force/Torque routines.
	if( strstr( params, "-ft"  ) ) {
		double dir[3] = { 0.0, 1.0, 0.0};

		apparatus->fWaitSubjectReady( "Offset.bmp", "*** Centered Grip Test ***\nRelease manipulandum and press OK." );
		apparatus->ShowStatus( "Acquiring offsets ...", "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error during file save." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "WaitSubjectReady.bmp", "Now pinch OFF center to desired force (see LEDs)." );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected.", "alert.bmp" );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitCenteredGrip( 25.0, 0.5, 1.0, "Off-center grip detected (25 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 20.0, 0.5, 1.0, "Off-center grip detected (20 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 15.0, 0.5, 1.0, "Off-center grip detected (15 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 10.0, 0.5, 1.0, "Off-center grip detected (10 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip(  5.0, 0.5, 1.0, "Off-center grip detected (5 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip(  1.0, 0.5, 1.0, "Off-center grip detected (1 mm).\nPress <IGNORE> to continue.", "info.bmp" );
		status = apparatus->fWaitSubjectReady( "ok.bmp", "If no other message, centered grip detected." );
		if ( status != NORMAL_EXIT ) return( status );

		apparatus->fWaitSubjectReady( "Offset.bmp", "Release manipulandum and press OK." );
		apparatus->ShowStatus( "Acquiring offsets ...", "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error saving data." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "WaitSubjectReady.bmp", "Now pinch in the center to desired force (see LEDs)." );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected.", "alert.bmp" );
		apparatus->fShowStatus( "working.bmp", "Maintian grip." );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitCenteredGrip( 25.0, 0.5, 1.0, "Off-center grip detected (25 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 15.0, 0.5, 1.0, "Off-center grip detected (15 mm).\nPMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 10.0, 0.5, 1.0, "Off-center grip detected (10 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip(  5.0, 0.5, 1.0, "Off-center grip detected (5 mm).\nPress <IGNORE> to continue.", "info.bmp" );
		status = apparatus->fWaitSubjectReady( "ok.bmp", "If no other message, centered grip detected." );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitSubjectReady("ok.bmp", "Protocol Terminated." );
			

	}

	if( strstr( params, "-slip"  ) ) {
		double dir[3] = { 0.0, 1.0, 0.0};

		apparatus->fWaitSubjectReady( "Offset.bmp", "Slip Test - Release manipulandum and press OK." );
		apparatus->ShowStatus( "Acquiring offsets ...", "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error during acquisition." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "WaitSubjectReady.bmp", "Now pinch in the center to desired force (see LEDs)." );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected in 15 sec.", "alert.bmp" );
		apparatus->Beep();
		for ( int i = 1; i <= 5; i++ ) {
			AnalysisProgress( apparatus, i, 5, "Keep rubbing. Need more slips." );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->fShowStatus( "WaitSubjectReady.bmp", "Now pull until slip." );
			if ( status != NORMAL_EXIT ) return( status );
			status = apparatus->WaitSlip( 2.0, 10.0, 0.0, 0.0, dir, 2.0, 0.5, 15.0, "No slip detected in 15 sec.", "alert.bmp" );
			apparatus->Beep();
			if ( status != NORMAL_EXIT ) return( status );
 			apparatus->Wait( 1.0 );
		}
		AnalysisProgress( apparatus, i, 5, "Success!" );
		status = apparatus->WaitSubjectReady("ok.bmp", "Protocol Terminated." );
			

	}

	// Check WaitUntilAtTarget().
	if( strstr( params, "-waits"  ) ) {
		int tgt;
		Vector3 v_tolerance = { 150.0, 12.0, 50.0 };
		Vector3 h_tolerance = { 150.0, 50.0, 12.0 };
		float timeout = 25.0;

		static int	direction = VERTICAL;
		static DexTargetBarConfiguration bar_position = TargetBarRight;
		static DexSubjectPosture posture = PostureSeated;
		
		// Seated or supine?
		posture = ParseForPosture( params );

		// Horizontal or vertical movements?
		direction = ParseForDirection( apparatus, params );
		if ( direction == VERTICAL ) bar_position = TargetBarRight;
		else bar_position = TargetBarLeft;

		status = CheckInstall( apparatus, posture, bar_position );		
		if ( status != NORMAL_EXIT ) return( status );
		
		if ( direction == VERTICAL ) {
			apparatus->ShowStatus( "", "MvToBlkV.bmp" );
			for ( tgt = 0; tgt < apparatus->nVerticalTargets; tgt++ ) {
				apparatus->fShowStatus( "MvToBlkV.bmp", "Move to blinking target.\n(Vertical Target %d)", tgt );
				status = apparatus->WaitUntilAtVerticalTarget( tgt, uprightNullOrientation, v_tolerance, defaultOrientationTolerance, 1.0, timeout, "Time out waiting to reach target.", "alert.bmp" );
				if ( status != NORMAL_EXIT ) return( status );
			}
		}
		else {
			apparatus->ShowStatus( "", "MvToBlkH.bmp" );
			for ( tgt = 0; tgt < apparatus->nHorizontalTargets; tgt++ ) {
				apparatus->fShowStatus( "MvToBlkH.bmp", "Move to blinking target.\n(Horizontal Target %d)", tgt );
				status = apparatus->WaitUntilAtHorizontalTarget( tgt, uprightNullOrientation, h_tolerance, defaultOrientationTolerance, 1.0, timeout, "Time out waiting to reach target.", "alert.bmp" );
				if ( status != NORMAL_EXIT ) return( status );
			}
		}
		status = apparatus->WaitSubjectReady("ok.bmp", "Protocol Terminated." );
	}

	// Check All Targets.
	if( strstr( params, "-LEDs"  ) ) {
		int tgt;
		apparatus->TargetsOff();
		status = apparatus->fWaitSubjectReady( "needpic.bmp", "Verify that all targets are OFF." );
		if ( status != NORMAL_EXIT ) return( status );
		for ( tgt = 0; tgt < apparatus->nVerticalTargets; tgt++ ) {
			apparatus->VerticalTargetOn( tgt );
			status = apparatus->fWaitSubjectReady( "MvToBlkV.bmp", "Verify that target is ON.\n(Vertical Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->VerticalTargetOff( tgt );
			status = apparatus->fWaitSubjectReady( "MvToBlkV.bmp", "Verify that target is OFF again.\n(Vertical Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
		}
		for ( tgt = 0; tgt < apparatus->nHorizontalTargets; tgt++ ) {
			apparatus->HorizontalTargetOn( tgt );
			status = apparatus->fWaitSubjectReady( "MvToBlkH.bmp", "Verify that target is ON.\n(Horizontal Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->HorizontalTargetOff( tgt );
			status = apparatus->fWaitSubjectReady( "MvToBlkH.bmp", "Verify that target is OFF again.\n(Horizontal Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
		}
		apparatus->SetTargetState( 0xFFFFFFFF );
		status = apparatus->fWaitSubjectReady( "needpic.bmp", "Verify that all targets are ON." );
		if ( status != NORMAL_EXIT ) return( status );

		apparatus->TargetsOff();

	}

	if( strstr( params, "-gm"  ) ) {
		status = apparatus->fWaitSubjectReady( "needpic.bmp", "Mass selection tests." );
		if ( status != NORMAL_EXIT ) return( status );
		apparatus->SelectAndCheckMass( MassSmall );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the lightest mass in your hand." );
		apparatus->SelectAndCheckMass( MassMedium );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the medium mass in your hand." );
		apparatus->SelectAndCheckMass( MassLarge );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the heaviest mass in your hand." );
		apparatus->SelectAndCheckMass( MassSmall );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the lightest mass in your hand." );
		apparatus->SelectAndCheckMass( MassMedium );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the medium mass in your hand." );
		apparatus->SelectAndCheckMass( MassLarge );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the heaviest mass in your hand." );
		status = apparatus->fWaitSubjectReady( "ok.bmp", "Test ended." );
	}


	// Record some data.
	if( strstr( params, "-rec"  ) ) {

		char *tag = "FREEDATA";

		double duration = ParseForDuration( apparatus, params );
		apparatus->fWaitSubjectReady( "confirm.bmp", "Ready to record for %.2f seconds. Press <OK> to start.", duration );
		apparatus->ShowStatus( "Acquiring.", "working.bmp" );
		apparatus->StartFilming( tag );
		apparatus->StartAcquisition( tag, duration );
		apparatus->Wait( duration );
		apparatus->StopAcquisition( "Error during saving." );
		apparatus->StopFilming();
		SignalEndOfRecording( apparatus );
		status = apparatus->fWaitSubjectReady( "ok.bmp", "Acquisition ended." );

	}

	return( status );

}

