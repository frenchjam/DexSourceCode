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
double	alignmentTolerance = 10.0;				// Allowable misalignment of marker positions between CODAs.
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
Vector3		expected_coda1_position_upright = { -800, 800, 2400};
Quaternion	expected_coda1_orientation_upright = {0.5, -0.5, 0.5, -0.5};

Vector3		expected_coda1_position_supine = { 800, 2600, 500};
Quaternion	expected_coda1_orientation_supine = {-0.707107, 0.0, .707107, 0.0};

// Coda Unit #2
Vector3		expected_coda2_position_upright = {     0, 1200, 2200};
Quaternion	expected_coda2_orientation_upright = {0.0, -0.707, 0.707, 0.0};

Vector3		expected_coda2_position_supine = { 0, 2600, 900};
Quaternion	expected_coda2_orientation_supine = { 1.0, 0.0, 0.0, 0.0};

// How close do we need to be to the expected position and orientation?
double codaUnitPositionTolerance = 900.0;		// Allowable displacement wrt expected position, in mm.
double codaUnitOrientationTolerance = 90.0;		// Allowable rotation wrt expected orientation, in degrees.

double codaUnitPositionRelaxed =  2000.0;		// Use this if all you really care about is upright vs. supine..
double codaUnitOrientationIgnore = 180.0;		// Use this if you don't care what the orientation is. Must be 180 or less.

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


	// Check which configuration is to be used and compute a tag accordingly.
	DexSubjectPosture desired_posture = ParseForPosture( params );
	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else {
		strcpy( tag, "GripCfg" );
		if ( desired_posture == PostureSeated ) strcat( tag, "U" ); // U is for upright (seated).
		else strcat( tag, "S" ); // S is for supine.
	}

	// Prompt the subject to install the apparatus accordingly.
	if ( desired_posture == PostureSeated ) status = apparatus->WaitSubjectReady("CalSeated.bmp", "Check Workspace Tablet installed for SEATED operation with Target Mast in socket N (right-hand side)." );
	else status = apparatus->WaitSubjectReady("CalSupine.bmp", "Check Workspace Tablet installed for SUPINE operation with Target Mast in socket N (right-hand side)." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum in the holder on the chair.
	status = apparatus->WaitSubjectReady("ManipInChair.bmp", "Place the manipulandum in the Manipulandum Calibration Cradle as shown. Check that locker door is fully open." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	status = apparatus->WaitSubjectReady("Visible.bmp", "Check that the 'VISIBLE' status indicator on the Workspace Tablet touchscreen is green." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Get pictures of the configuration during the tests.
	apparatus->StartFilming( tag, defaultCameraFrameRate );

	// Run the self test.
	status = apparatus->SelfTest();
	if ( status != NORMAL_EXIT ) return( status );

	// Check that the 4 reference markers and the manipulandum are in the ideal field-of-view of each Coda unit.
	apparatus->ShowStatus( "Checking field of view ...", "wait.bmp" );
	status = apparatus->CheckTrackerFieldOfView( 0, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
		"Tracking Camera #1 off center.\n- Check boresighting.\n- Check reference markers in view.\nCorrect and <Retry> or call COL-CC.", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	if ( apparatus->nCodas > 1 ) {
		status = apparatus->CheckTrackerFieldOfView( 1, fovMarkerMask, fov_min_x, fov_max_x, fov_min_y, fov_max_y, fov_min_z, fov_max_z,
			"Tracking Camera #2 off center.\n- Check boresighting.\n- Check reference markers in view.\nCorrect and <Retry> or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	}

	// We used to move the mast to the left (standby) position to improve the alignment, but that has proved problematic for visibility.
	// So now we leave it on the right.
	// Prompt the subject to place the target bar in the left side position.
	// status = apparatus->WaitSubjectReady("BarLeft.bmp", "Move the Target Mast to the Standby socket (left-hand side)." );
	// if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform the alignment based on those markers.
	apparatus->ShowStatus( "Performing alignment ...", "wait.bmp" );
	status = apparatus->PerformTrackerAlignment( "Tracker alignment error.\n- Check reference markers in view.\nCorrect and <Retry> or call COL-CC.", "alert.bmp" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	apparatus->ShowStatus( "Checking tracker placement ...", "wait.bmp" );
	if ( desired_posture == PostureSeated ) {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionRelaxed, 
											expected_coda1_orientation_upright, codaUnitOrientationIgnore, 
											"Placement error - Tracker Camera 1\n- Check configured for SEATED\nCorrect and <Retry> or call COL-CC.", "CheckSeatedR.bmp" );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionRelaxed, 
											expected_coda1_orientation_supine, codaUnitOrientationIgnore, 
											"Placement error - Tracker Camera 1\n- Check configured for SUPINE\nCorrect and <Retry> or call COL-CC.", "CheckSupineR.bmp" );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );


	if ( apparatus->nCodas > 1 ) {

		if ( desired_posture == PostureSeated ) {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionRelaxed, 
												expected_coda2_orientation_upright, codaUnitOrientationIgnore, 
												"Placement error - Tracker Camera 2\n- Check configured for SEATED\nCorrect and <Retry> or call COL-CC.", "CheckSeatedR.bmp" );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionRelaxed, 
												expected_coda2_orientation_supine, codaUnitOrientationIgnore, 
												"Placement error - Tracker Camera 2\n- Check configured for SUPINE\nCorrect and <Retry> or call COL-CC.", "CheckSupineR.bmp" );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

		// Check that the trackers are still aligned with each other.
		apparatus->ShowStatus( "Checking tracker alignment ...", "wait.bmp" );
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, 
			"Tracker Camera misalignment detected!\n- Check reference markers in view.\nCorrect and <Retry> or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	}
	

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->Wait( alignmentAcquisitionDuration );
	apparatus->StopAcquisition( "Error during file save." );
	apparatus->StopFilming();

	apparatus->ShowStatus( "Checking visibility ...", "wait.bmp" );
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

	int status = NORMAL_EXIT;

	
#ifdef INHIBIT_ALL_INSTALLATION_CHECKS
	return status;
#endif

	// Tune the picture depending on the expected position of the bar.
	// Only applies to seated, because the pictures don't really show the bar position in the supine case.
	char *seated_pic = "CheckSeatedR.bmp";
	if ( desired_bar_position == TargetBarLeft ) seated_pic = "CheckSeatedL.bmp";
	char *supine_pic = "CheckSupineR.bmp";
	if ( desired_bar_position == TargetBarLeft ) supine_pic = "CheckSupineL.bmp";

	apparatus->Comment( "################################################################################" );
	apparatus->Comment( "Operations to check the hardware configuration." );

	// Get pictures of the configuration during the tests.
	apparatus->StartFilming( "GripCHCK", defaultCameraFrameRate );

	apparatus->ShowStatus( "Checking hardware configuration ...", "wait.bmp" );

#ifndef INHIBIT_PLACEMENT_CHECKS

	// Are the CODA bars where we think they should be, given the desired postural configuration?
	// Here we are mainly concerned with checking if the alignment was done in the desired configuration.
	// We don't care so much if the CODAs are swapped or installed in a different configuration.
	// So we use a relaxed constraint on the position of each unit and we ignore the orientation of the units.

	if ( desired_posture == PostureSeated ) {

		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_upright, codaUnitPositionRelaxed, 
											expected_coda1_orientation_upright, codaUnitOrientationIgnore, 
											"Unexpected Configuration.\n- Check configured for SEATED\nCorrect and <Retry> or call COL-CC.", seated_pic );
	}
	else {
		status = apparatus->CheckTrackerPlacement( 0, 
											expected_coda1_position_supine, codaUnitPositionRelaxed, 
											expected_coda1_orientation_supine, codaUnitOrientationIgnore, 
											"Unexpected Configuration.\n- Check configured for SUPINE\nCorrect and <Retry> or call COL-CC.", supine_pic );

	}
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
#endif


	if ( apparatus->nCodas > 1 ) {

#ifndef INHIBIT_PLACEMENT_CHECKS
		if ( desired_posture == PostureSeated ) {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_upright, codaUnitPositionRelaxed, 
												expected_coda2_orientation_upright, codaUnitOrientationIgnore, 
												"Unexpected Configuration.\n- Check configured for SEATED\nCorrect and <Retry> or call COL-CC.", seated_pic );
		}
		else {
			status = apparatus->CheckTrackerPlacement( 1, 
												expected_coda2_position_supine, codaUnitPositionRelaxed, 
												expected_coda2_orientation_supine, codaUnitOrientationIgnore, 
												"Unexpected Configuration.\n- Check configured for SUPINE\nCorrect and <Retry> or call COL-CC.", supine_pic );

		}
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
#endif

#ifndef INHIBIT_ALIGNMENT_CHECKS
		// Check that the trackers are still aligned with each other.
		status = apparatus->CheckTrackerAlignment( alignmentMarkerMask, alignmentTolerance, alignmentRequiredGood, 
			"Coda misalignment detected!\n- Are reference markers in view?\\nCorrect and <Retry> or call COL-CC.", "alert.bmp" );
		if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
#endif

	}
		

#ifndef INHIBIT_MAST_CHECKS
	// The current implementation of SelectAndCheckConfiguration() does not handle doing the aligment in the supine configuration. 
	// The test to decide if the hdw is in the upright or supine configuration assumes that the alignment was done in the upright 
	//  configuration. If the alignment is done in the supine configuration, then the test will say that it is in the upright 
	//  configuration when the apparatus is in the supine configuration and vice versa.
	// Not to fear. The tests of the tracker placement above have already determined that the hardware was installed in the proper
	//  configuation and that the alignment was done in that configuration. So what we do here is set the desired configuration 
	//  to be upright (seated), which will appear to be true in both the supine and upright cases. Nevertheless, the error message
	//  and picture displayed to the subject if the test fails should be what we really want.
	if ( desired_posture == PostureSupine ) {
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( supine_pic, "Unexpected Configuration.\n- Check configured for SUPINE\n- Check target mast on LEFT\n- Check reference markers visible", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( supine_pic, "Unexpected Configuration.\n- Check configured for SUPINE\n- Check target mast on RIGHT\n- Check reference markers visible\n", PostureSeated, desired_bar_position, DONT_CARE );
	}
	else {
		if ( desired_bar_position == TargetBarLeft ) status = apparatus->SelectAndCheckConfiguration( seated_pic, "Unexpected Configuration.\n- Check configured for SEATED\n- Check target mast on LEFT\n- Check reference markers visible", PostureSeated, desired_bar_position, DONT_CARE );
		else status = apparatus->SelectAndCheckConfiguration( seated_pic, "Unexpected Configuration.\n- Check configured for SEATED\n- Check target mast on RIGHT\n- Check reference markers visible", PostureSeated, desired_bar_position, DONT_CARE );
	}
#endif
	
	apparatus->StopFilming();
	apparatus->HideStatus();

	return( status );
}

/************************************************************************************************************************************/

// Instruct the subject to adjust the audio volume so they are sure to hear the tones.

int CheckAudio( DexApparatus *apparatus, const char *params ) {

	int status;

	status = apparatus->WaitSubjectReady("headphones.bmp", "Don the headphones and connect to the Grip hardware." );
	if ( status == ABORT_EXIT ) return( status );

	char tag[32];
	if ( ParseForTag( params ) ) strcpy( tag, ParseForTag( params ) );
	else strcpy( tag, "GripSOND" );
	apparatus->StartAcquisition( tag, maxTrialDuration );
	apparatus->StartFilming( tag, defaultCameraFrameRate );
	
	char msg[1024];
	sprintf( msg, "A series of beeps is being played for %.0f seconds. Adjust the audio control until you hear them clearly.", audioCheckDuration );
	apparatus->ShowStatus( msg, "volume.bmp" );
	for ( int i = 0; i < audioCheckDuration; i++ ) {
		apparatus->Beep();
		apparatus->Wait( 0.9 );
	}
	apparatus->StopFilming();
	apparatus->StopAcquisition( "Error during file save." );

	apparatus->ShowStatus( "", "blank.bmp" );
	status = apparatus->WaitSubjectReady("confirm.bmp", "Test complete. If you did not hear the beeps, check the hardware and then repeat this task." );
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

		status = apparatus->fWaitSubjectReady("confirm.bmp", "Confirm Subject ID: %s\n\nIf not correct, press <OK>, then <Back>, then <Logout> and select your ID.", id );
		if ( status != NORMAL_EXIT ) return( status );
	}

	// Place this at the end of a protocol to let the subject know that he or she is finished.
	if( strstr( params, "-finish"  ) ) {
		status = apparatus->WaitSubjectReady("ok.bmp", "Session completed.\nPress <OK> and go back to PODF." );
		if ( status != NORMAL_EXIT ) return( status );
	}

	////// Hardware Tests ////////

	// Run the self test in isolation.
	if( strstr( params, "-selftest"  ) ) {

		apparatus->fWaitSubjectReady( "info.bmp", "********* Run Self Test *********\nPress <OK> to start test." );

		status = apparatus->SelfTest();
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitSubjectReady("ok.bmp", "Test completed." );

	}


	// Check Force/Torque routines.
	if( strstr( params, "-ft"  ) ) {
		double dir[3] = { 0.0, 1.0, 0.0};

		apparatus->fWaitSubjectReady( "REMOVE_HAND.bmp", "********* Centered Grip Test *********\nRelease manipulandum and Press <OK>." );
		apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error during file save." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "ShowPinch.bmp", "Pinch away from the center until the desired force is achieved. (Watch blinking Target LED.)" );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected.", "alert.bmp" );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitCenteredGrip( 25.0, 0.5, 1.0, "Off-center grip detected (25 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 20.0, 0.5, 1.0, "Off-center grip detected (20 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 15.0, 0.5, 1.0, "Off-center grip detected (15 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip( 10.0, 0.5, 1.0, "Off-center grip detected (10 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip(  5.0, 0.5, 1.0, "Off-center grip detected (5 mm).\nMaintain grip and press <IGNORE>.", "info.bmp" );
		status = apparatus->WaitCenteredGrip(  1.0, 0.5, 1.0, "Off-center grip detected (1 mm).\nPress <IGNORE> to continue.", "info.bmp" );
		status = apparatus->fWaitSubjectReady( "info.bmp", "If no other messages, a centered grip was detected." );
		if ( status != NORMAL_EXIT ) return( status );

		apparatus->fWaitSubjectReady( "REMOVE_HAND.bmp", MsgReleaseAndOK );
		apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error saving data." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "ShowPinch.bmp", "Now pinch in the center until the desired force is achieved (see LEDs)." );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected.", "alert.bmp" );
		apparatus->fShowStatus( "working.bmp", "Maintian grip." );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitCenteredGrip( 25.0, 0.5, 1.0, "Off-center grip detected (25 mm).\nMaintain grip and press <IGNORE>.", "ok.bmp" );
		status = apparatus->WaitCenteredGrip( 15.0, 0.5, 1.0, "Off-center grip detected (15 mm).\nPMaintain grip and press <IGNORE>.", "ok.bmp" );
		status = apparatus->WaitCenteredGrip( 10.0, 0.5, 1.0, "Off-center grip detected (10 mm).\nMaintain grip and press <IGNORE>.", "ok.bmp" );
		status = apparatus->WaitCenteredGrip(  5.0, 0.5, 1.0, "Off-center grip detected (5 mm).\nPress <IGNORE> to continue.", "ok.bmp" );
		status = apparatus->fWaitSubjectReady( "info.bmp", "If no other messages, a centered grip was detected." );
		if ( status != NORMAL_EXIT ) return( status );
		status = apparatus->WaitSubjectReady("ok.bmp", "Test completed." );
			

	}

	if( strstr( params, "-slip"  ) ) {
		double dir[3] = { 0.0, 1.0, 0.0};

		apparatus->fWaitSubjectReady( "REMOVE_HAND.bmp", "*********     Slip Test     *********\nRelease manipulandum and Press <OK>." );
		apparatus->ShowStatus( MsgAcquiringBaseline, "wait.bmp" );
		apparatus->StartAcquisition( "FTChk", maxTrialDuration );
		apparatus->Wait( 0.5 );
		apparatus->ShowStatus( "Saving data ...", "wait.bmp" );
		apparatus->StopAcquisition( "Error during acquisition." );

		apparatus->ShowStatus( "Processing data ...", "wait.bmp" );
		// Compute the offsets and insert them into force calculations.
		apparatus->ComputeAndNullifyStrainGaugeOffsets();
		apparatus->ShowStatus( "Force offsets nullified.", "ok.bmp" );

		apparatus->fShowStatus( "ShowPinch.bmp", "Now pinch in the center until the desired force is achieved (see LEDs)." );
		status = apparatus->WaitDesiredForces( 2.0, 10.0, 0.0, 0.0, dir, 4.0, 0.1, 15.0, "No grip detected.", "alert.bmp" );
		apparatus->Beep();
		for ( int i = 1; i <= 5; i++ ) {
			AnalysisProgress( apparatus, i, 5, "Keep rubbing. Need more slips." );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->fShowStatus( "ShowPinch.bmp", "Now pull until slip." );
			if ( status != NORMAL_EXIT ) return( status );
			status = apparatus->WaitSlip( 2.0, 10.0, 0.0, 0.0, dir, 2.0, 0.5, 15.0, "No slip detected.", "alert.bmp" );
			apparatus->Beep();
			if ( status != NORMAL_EXIT ) return( status );
 			apparatus->Wait( 1.0 );
		}
		AnalysisProgress( apparatus, i, 5, "Completed." );
		status = apparatus->WaitSubjectReady("ok.bmp", "Task completed." );
			

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
//			apparatus->ShowStatus( "", "MvToBlkV.bmp" );
			for ( tgt = 0; tgt < apparatus->nVerticalTargets; tgt++ ) {
				apparatus->fShowStatus( "MvToBlkV.bmp", "Move to blinking target.\n(Vertical Target %d)", tgt + 1 );
				status = apparatus->WaitUntilAtVerticalTarget( tgt, uprightNullOrientation, v_tolerance, defaultOrientationTolerance, 1.0, timeout, "Too long to reach blinking Target LED.\nPress <Retry> to try again.", "alert.bmp" );
				if ( status != NORMAL_EXIT ) return( status );
			}
		}
		else {
//			apparatus->ShowStatus( "", "MvToBlkH.bmp" );
			for ( tgt = 0; tgt < apparatus->nHorizontalTargets; tgt++ ) {
				apparatus->fShowStatus( "MvToBlkH.bmp", "Move to blinking target.\n(Horizontal Target %d)", tgt + 1 );
				status = apparatus->WaitUntilAtHorizontalTarget( tgt, uprightNullOrientation, h_tolerance, defaultOrientationTolerance, 1.0, timeout, "Too long to reach blinking Target LED.\nPress <Retry> to try again.", "alert.bmp" );
				if ( status != NORMAL_EXIT ) return( status );
			}
		}
		status = apparatus->WaitSubjectReady("ok.bmp", "Test completed." );
	}

	// Check All Targets.
	if( strstr( params, "-LEDs"  ) ) {
		int tgt;
		apparatus->TargetsOff();
		status = apparatus->fWaitSubjectReady( "AllTgtOff.bmp", "***** Target LED Hardware Tests  ****\nVerify that all targets are OFF.\nPress <OK> to continue." );
		if ( status != NORMAL_EXIT ) return( status );
		for ( tgt = 0; tgt < apparatus->nVerticalTargets; tgt++ ) {
			apparatus->VerticalTargetOn( tgt );
			status = apparatus->fWaitSubjectReady( "VertTgts.bmp", "Verify that Target LED is ON.\n(Vertical Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->VerticalTargetOff( tgt );
			status = apparatus->fWaitSubjectReady( "VertTgts.bmp", "Verify that target is OFF again.\n(Vertical Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
		}
		for ( tgt = 0; tgt < apparatus->nHorizontalTargets; tgt++ ) {
			apparatus->HorizontalTargetOn( tgt );
			status = apparatus->fWaitSubjectReady( "HoriTgts.bmp", "Verify that Target LED is ON.\n(Horizontal Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
			apparatus->HorizontalTargetOff( tgt );
			status = apparatus->fWaitSubjectReady( "HoriTgts.bmp", "Verify that Target LED is OFF again.\n(Horizontal Target %d)", tgt + 1 );
			if ( status != NORMAL_EXIT ) return( status );
		}
		apparatus->SetTargetState( 0xFFFFFFFF );
		status = apparatus->fWaitSubjectReady( "AllTgtOn.bmp", "Verify that all Target LEDs are ON." );
		if ( status != NORMAL_EXIT ) return( status );
		apparatus->TargetsOff();
		status = apparatus->fWaitSubjectReady( "ok.bmp", "Test completed." );

	}

	if( strstr( params, "-gm"  ) ) {
		status = apparatus->fWaitSubjectReady( "RetainerManip.bmp", "*** Mass Selection Hardware Tests *** Manipulandum should be in retainer. Press <OK> to start." );
		if ( status != NORMAL_EXIT ) return( status );

		status = apparatus->SelfTest();
		if ( status != NORMAL_EXIT ) return( status );

		apparatus->SelectAndCheckMass( MassSmall );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the lightest mass in your hand. Shake it a bit to get a feel for its inertia." );
		apparatus->SelectAndCheckMass( MassMedium );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the medium mass in your hand. Notify COL-CC if inertia does not feel GREATER than previous." );
		apparatus->SelectAndCheckMass( MassLarge );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the heaviest mass in your hand. Notify COL-CC if inertia does not feel GREATER than previous." );
		apparatus->SelectAndCheckMass( MassSmall );
		status = apparatus->fWaitSubjectReady( "info.bmp", "You should have the lightest mass in your hand again. Notify COL-CC if inertia does not feel LOWER than previous." );
		status = apparatus->WaitSubjectReady("PlaceMass.bmp", "Place the mass from the manipulandum in the empty cradle and lock in place." );
		status = apparatus->WaitSubjectReady("RetainerManip.bmp", "Return the manipulandum without mass to the retainer as shown and lock in place (push pins)." );
	}


	// Record some data.
	if( strstr( params, "-rec"  ) ) {

		char *tag = "FREEDATA";

		double duration = ParseForDuration( apparatus, params );
		apparatus->fWaitSubjectReady( "go.bmp", "Ready to record for %.0f seconds.\nPress <OK> to start.", duration );
		apparatus->ShowStatus( "Acquiring ...", "working.bmp" );
		apparatus->StartFilming( tag, defaultCameraFrameRate );
		apparatus->StartAcquisition( tag, duration );
		apparatus->Wait( duration );
		apparatus->StopAcquisition( "Error during saving." );
		apparatus->StopFilming();
		SignalEndOfRecording( apparatus );
		status = apparatus->fWaitSubjectReady( "ok.bmp", "Acquisition completed." );

	}

	return( status );

}

