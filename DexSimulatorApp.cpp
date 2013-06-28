/*********************************************************************************/
/*                                                                               */
/*                             DexSimulatorApp.cpp                               */
/*                                                                               */
/*********************************************************************************/

/*
 * Demonstrate the principle steps that need to be accomplished in a DEX protocol.
 * Demonstrate the concept of a script compiler and interpreter.
 * Demonstrate how to communicate with the CODA.
 */

/*********************************************************************************/

//#include "stdafx.h"
#include "..\DexSimulatorApp\resource.h"

#include <Winsock2.h>

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"

#include <3dMatrix.h>
#include <OglDisplayInterface.h>
#include <Views.h>
#include <Layouts.h>

/*********************************************************************************/

#define SKIP_PREP	// Skip over some of the setup checks just to speed up debugging.
#define AUTOSCALE	// Autoscale the data traces.

/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, DexTargetBarConfiguration bar_position, int target_sequence[], int n_targets  );
int RunOscillations( DexApparatus *apparatus );
int RunCollisions( DexApparatus *apparatus );
int RunScript( DexApparatus *apparatus, const char *filename );

/*********************************************************************************/

// Common paramters.
double baselineTime = 1.0;					// Duration of pause at first target before starting movement.
double continuousDropoutTimeLimit = 0.050;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
double cumulativeDropoutTimeLimit = 1.000;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
int desired_posture = PostureSeated;
double beepTime = BEEP_DURATION;

// Force offset parameters.
double offsetMaxTrialTime = 120.0;		// Max time to perform the whole list of movements. Set to 12 to simulate error.
double offsetAcquireTime = 2.0;			// How long to acquire when computing strain gauge offsets.

// Coefficient of friction test parameters.
double frictionHoldTime = 2.0;
double frictionTimeout = 15.0;
double frictionMinGrip = 10.0;
double frictionMaxGrip = 15.0;
double frictionMinLoad = 5.0;
double frictionMaxLoad = 10.0;
double forceFilterConstant = 1.0;
Vector3 frictionLoadDirection = { 0.0, -1.0, 0.0 };

double copTolerance = 10.0;
double slipThreshold = 10.0;
double slipTimeout = 20.0;

// Targeted trial parameters;
int targetSequence[] = { 0, 1, 0, 4, 0, 2, 0, 3, 0 };	// List of targets for point-to-point movements.
int targetSequenceN = 2;
double movementTime = 2.0;					// Time allowed for each movement.
double targetedMaxTrialTime = 120.0;		// Max time to perform the whole list of movements. Set to 12 to simulate error.
double targetedMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double targetedMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.

// Oscillation trial parameters.
int oscillationUpperTarget = 9;						// Targets showing desired amplitude of cyclic movement.
int oscillationLowerTarget = 3;
int oscillationCenterTarget = 6;
double oscillationTime = 10.0;
double oscillationMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double oscillationMinMovementExtent = 15.0;	// Minimum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
double oscillationMaxMovementExtent = HUGE;	// Maximum amplitude along the movement direction (Y). Set to 1000.0 to simulate error.
int	oscillationMinCycles = 6;	// Minimum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
int oscillationMaxCycles = 0;	// Maximum cycles along the movement direction (Y). Set to 1000.0 to simulate error.
float cycleHysteresis = 10.0;

// Collision trial parameters;
int collisionInitialTarget = 6;
int collisionUpTarget = 11;
int collisionDownTarget = 1;

#define UP		0
#define DOWN	1
int collisionSequenceN = 3;
int collisionSequence[] = { DOWN, UP, UP, DOWN, DOWN, DOWN, UP, DOWN, UP, UP };
double collisionTime = 2.0;
double collisionMaxTrialTime = 120.0;		// Max time to perform the whole list of movements.
double collisionMovementThreshold = 10.0;

int exit_status = NORMAL_EXIT;

Vector3 expected_position[2] = {{-1000.0, 0.0, 2500.0}, {0.0, 900.0, 2500.0}};
Quaternion expected_orientation[2];
unsigned int RefMarkerMask = 0x000f0000;

float flashTime = 0.1;

// 2D Graphics
Display display; 
Layout  layout;
View	view, yz_view, cop_view;

int plot_screen_left = 225;
int plot_screen_top = 120;
int plot_screen_width = 772;
int plot_screen_height = 600;
float _plot_z_min = -500.0;
float _plot_z_max =  500.0;
float _plot_y_min = -200.0;
float _plot_y_max =  800.0;
int   _n_plots = 6;

float cop_range = 0.03;
float load_range = 100.0;
float grip_range = 25.0;

HWND	dlg;
HWND	saving_dlg;

/*********************************************************************************/

void BlinkAll ( DexApparatus *apparatus ) {

	apparatus->SetTargetState( ~0 );
	apparatus->Wait( flashTime );
	apparatus->TargetsOff();
	apparatus->Wait( flashTime );

}

/**************************************************************************************/

void init_plots ( void ) {

	display = DefaultDisplay();
	DisplaySetSizePixels( display, plot_screen_width, plot_screen_height );
	DisplaySetScreenPosition( display, plot_screen_left, plot_screen_top );
	DisplaySetName( display, "DEX Monitor - Recorded Data" );
	DisplayInit( display );
	Erase( display );
	
	yz_view = CreateView( display );
	ViewSetDisplayEdgesRelative( yz_view, 0.01, 0.51, 0.30, 0.99 );
	ViewSetEdges( yz_view, _plot_z_min, _plot_y_min, _plot_z_max, _plot_y_max );
	ViewMakeSquare( yz_view );
	
	cop_view = CreateView( display );
	ViewSetDisplayEdgesRelative( cop_view, 0.01, 0.01, 0.30, 0.49 );
	ViewSetEdges( cop_view, - cop_range, - cop_range, cop_range, cop_range );
	ViewMakeSquare( cop_view );

	layout = CreateLayout( display, _n_plots, 1 );
	LayoutSetDisplayEdgesRelative( layout, 0.31, 0.01, 0.99, 0.99 );

	ActivateDisplayWindow();
	Erase( display );
	DisplaySwap( display );
	HideDisplayWindow();

}

void plot_data( DexApparatus *apparatus ) {

	int i, j, cnt;

	ShowDisplayWindow();
	ActivateDisplayWindow();
	Erase( display );
		
	ViewColor( yz_view, GREY4 );
	ViewBox( yz_view );
	ViewBox( cop_view );
	ViewCircle( cop_view, 0.0, 0.0, copTolerance / 1000.0 );
	
	int frames = apparatus->nAcqFrames;
	int samples = apparatus->nAcqSamples;
	if ( frames > 0 ) {

		ViewColor( yz_view, RED );
		ViewPenSize( yz_view, 5 );
		ViewXYPlotAvailableFloats( yz_view,
			&apparatus->acquiredManipulandumState[0].position[Z], 
			&apparatus->acquiredManipulandumState[0].position[Y], 
			0, frames - 1, 
			sizeof( *apparatus->acquiredManipulandumState ), 
			sizeof( *apparatus->acquiredManipulandumState ), 
			INVISIBLE );

		for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
			ViewSelectColor( cop_view, i );
			ViewPenSize( cop_view, 5 );
			ViewXYPlotAvailableFloats( cop_view,
				&apparatus->acquiredCOP[i][0][X], 
				&apparatus->acquiredCOP[i][0][Y], 
				0, samples - 1, 
				sizeof( *apparatus->acquiredCOP[i] ), 
				sizeof( *apparatus->acquiredCOP[i] ), 
				INVISIBLE );
		}

		for ( i = 0, cnt = 0; i < 3; i++, cnt++ ) {
		
			view = LayoutViewN( layout, cnt );
			ViewColor( view, GREY4 );	
			ViewBox( view );
		
			ViewSetXLimits( view, 0.0, frames );
			ViewAutoScaleInit( view );
			ViewAutoScaleAvailableFloats( view,
				&apparatus->acquiredManipulandumState[0].position[i], 
				0, frames - 1, 
				sizeof( *apparatus->acquiredManipulandumState ), 
				INVISIBLE );
			ViewAutoScaleSetInterval( view, 500.0 );			
			
			ViewSelectColor( view, i );
			ViewPlotAvailableFloats( view,
				&apparatus->acquiredManipulandumState[0].position[i], 
				0, frames - 1, 
				sizeof( *apparatus->acquiredManipulandumState ), 
				INVISIBLE );
			
		}

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
	
		ViewSetXLimits( view, 0.0, samples );
		ViewAutoScaleInit( view );
		ViewAutoScaleAvailableFloats( view,
			&apparatus->acquiredGripForce[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredGripForce ), 
			INVISIBLE );
		ViewAutoScaleSetInterval( view, grip_range );			
		
		ViewColor( view, RED );
		ViewPlotAvailableFloats( view,
			&apparatus->acquiredGripForce[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredGripForce ), 
			INVISIBLE );

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
	
		ViewSetXLimits( view, 0.0, samples );
		ViewAutoScaleInit( view );
		ViewAutoScaleAvailableFloats( view,
			&apparatus->acquiredLoadForceMagnitude[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredLoadForceMagnitude ), 
			INVISIBLE );
		ViewAutoScaleSetInterval( view, load_range );			
		
		ViewColor( view, BLUE );
		ViewPlotAvailableFloats( view,
			&apparatus->acquiredLoadForceMagnitude[0], 
			0, samples - 1, 
			sizeof( *apparatus->acquiredLoadForceMagnitude ), 
			INVISIBLE );

		view = LayoutViewN( layout, cnt++ );
		ViewColor( view, GREY4 );	
		ViewBox( view );
	
		ViewSetXLimits( view, 0.0, samples );
		ViewSetYLimits( view, - 2.0 * copTolerance / 1000.0, 2.0 * copTolerance / 1000.0 );
		ViewLine( view, 0.0,   copTolerance / 1000.0, samples,   copTolerance / 1000.0 );
		ViewLine( view, 0.0, - copTolerance / 1000.0, samples, - copTolerance / 1000.0 );
		for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
			for ( j = 0; j < 2; j++ ) {
				ViewSelectColor( view, i * N_FORCE_TRANSDUCERS + j );
					ViewPlotAvailableFloats( view,
					&apparatus->acquiredCOP[i][0][j], 
					0, samples - 1, 
					sizeof( *apparatus->acquiredCOP[i] ), 
					INVISIBLE );
			}
		}
	}
	
	DisplaySwap( display );

	RunWindow();
	HideDisplayWindow();


}

/*********************************************************************************/

void save_data ( DexApparatus *apparatus, const char *tag ) {
	ShowWindow( saving_dlg, SW_SHOW );
	SetDlgItemText( saving_dlg, IDC_SAVING_CAPTION, "Saving data ..." );
	apparatus->SaveAcquisition( tag );
	ShowWindow( saving_dlg, SW_HIDE );
}

/*********************************************************************************/

int RunInstall( DexApparatus *apparatus ) {

	int status;

	// Express the expected orientation of each fo the CODA units as a quaternion.
	apparatus->SetQuaterniond( expected_orientation[0], 90.0, apparatus->kVector );
	apparatus->SetQuaterniond( expected_orientation[1],  0.0, apparatus->iVector );

	// Check that the 4 reference markers are in the ideal field-of-view of each Coda unit.
	status = apparatus->CheckTrackerFieldOfView( 0, RefMarkerMask, -1000.0, 1000.0, -1000.0, 1000.0, 2000.0, 4000.0 );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	status = apparatus->CheckTrackerFieldOfView( 1, RefMarkerMask, -1000.0, 1000.0, -1000.0, 1000.0, 2000.0, 4000.0 );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform the alignment based on those markers.
	status = apparatus->PerformTrackerAlignment();
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Are the Coda bars where we think they should be?
	status = apparatus->CheckTrackerPlacement( 0, 
										expected_position[0], 45.0, 
										expected_orientation[0], 45.0, 
										"Placement error - Coda Unit 0." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	status = apparatus->CheckTrackerPlacement( 1, 
										expected_position[1], 10.0, 
										expected_orientation[1], 10.0, 
										"Placement error - Coda Unit 1." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that the tracker is still aligned.
	status = apparatus->CheckTrackerAlignment( RefMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Prompt the subject to place the manipulandum on the chair.
	status = apparatus->WaitSubjectReady( "Place maniplandum in specified position on the chair." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Perform a short acquisition to measure where the manipulandum is.
	apparatus->StartAcquisition( 5.0 );
	apparatus->Wait( 5.0 );
	apparatus->StopAcquisition();
	apparatus->SaveAcquisition( "ALGN" );

	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	return( NORMAL_EXIT );
}

/*********************************************************************************/

int RunTransducerOffsetCompensation( DexApparatus *apparatus ) {

	int status;

	status = apparatus->WaitSubjectReady( "Place manipulandum in holder.\n\n  !!! REMOVE HAND !!!\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	// Acquire some data.
	apparatus->StartAcquisition( targetedMaxTrialTime );
	apparatus->Wait( offsetAcquireTime );
	apparatus->StopAcquisition();
	save_data( apparatus, "OFFS" );

	// Compute the offsets and insert them into force calculations.
	apparatus->ComputeAndNullifyStrainGaugeOffsets();

	return( NORMAL_EXIT );

}

/*********************************************************************************/

int RunFrictionMeasurement( DexApparatus *apparatus ) {

	int status;

	// Start acquiring Data.
	// Note: DexNiDaqADC must be run in polling mode so that we can both record the 
	// continuous data, but all monitor the COP in real time.
	// The following routine does that, but it will have no effect on the real
	//  DEX apparatus, which in theory can poll and sample continuously at the same time.
	apparatus->adc->AllowPollingDuringAcquisition();
	apparatus->StartAcquisition( targetedMaxTrialTime );

	status = apparatus->WaitSubjectReady( "Squeeze the manipulandum between thumb and index.\nPull upward.\nAdjust pinch and pull forces according to LEDs.\nWhen you hear the beep, pull harder until slippage.\n\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) return( status );

	status = apparatus->WaitCenteredGrip( copTolerance, 1.0, 1.0, "Grip not centered." );
	if ( status == ABORT_EXIT ) exit( status );

	apparatus->WaitDesiredForces( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, frictionHoldTime, 1000 * frictionTimeout );
	apparatus->MarkEvent( FORCE_OK );
	apparatus->Beep();
	apparatus->WaitSlip( frictionMinGrip, frictionMaxGrip, 
		frictionMinLoad, frictionMaxLoad, frictionLoadDirection, 
		forceFilterConstant, slipThreshold, slipTimeout );
	apparatus->MarkEvent( SLIP_OK );

	apparatus->Beep();
	apparatus->Wait( 0.25 );
	apparatus->Beep();
	apparatus->Wait( 2.0 );

	apparatus->StopAcquisition();
	save_data( apparatus, "FRIC" );

	return( NORMAL_EXIT );

}
/*********************************************************************************/

int RunTargeted( DexApparatus *apparatus, int direction, int target_sequence[], int n_targets ) {
	
	int status = 0;

	int bar_position;

	if ( direction == VERTICAL ) bar_position = TargetBarRight;
	else bar_position = TargetBarLeft;

#ifndef SKIP_PREP

	// Check that the tracker is still aligned.
	status = apparatus->CheckTrackerAlignment( RefMarkerMask, 5.0, 2, "Coda misaligned!" );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[bar_position], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( PostureSeated, bar_position, DONT_CARE );
	if ( status == ABORT_EXIT ) exit( status );

	// I am calling this method separately, but it could be incorporated into SelectAndCheckConfiguration().
	apparatus->SetTargetPositions();
	
	// Send information about the actual configuration to the ground.
	// This is redundant, because the SelectAndCheckConfiguration() command will do this as well.
	// But I want to demonstrate that it can be used independent from the check as well.
	apparatus->SignalConfiguration();
	
	// Instruct subject to take the appropriate position in the apparatus
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Take a seat and attach the belts.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

#endif

	// Instruct subject to pick up the manipulandum
	//  and wait for confimation that he or she is ready.
	status = apparatus->WaitSubjectReady( "Pick up the manipulandum in the right hand.\nBe sure that thumb and forefinger are centered.\nPress OK when ready to continue." );
	if ( status == ABORT_EXIT ) exit( status );

	// Check that the grip is properly centered.
	status = apparatus->WaitCenteredGrip( 10.0, 0.25, 1.0 );
	if ( status == ABORT_EXIT ) exit( status );

	// Start acquiring data.
	apparatus->StartAcquisition( targetedMaxTrialTime );

	// Wait until the subject gets to the target before moving on.
	if ( direction == VERTICAL ) status = apparatus->WaitUntilAtVerticalTarget( target_sequence[0], uprightNullOrientation );
	else status = apparatus->WaitUntilAtHorizontalTarget( target_sequence[0], supineNullOrientation, defaultPositionTolerance, 1.0 ); 
	if ( status == ABORT_EXIT ) exit( status );

	// Make sure that the target is turned back on if a timeout occured.
	// Normally this won't do anything.
	apparatus->TargetsOff();
	if ( direction == VERTICAL ) apparatus->VerticalTargetOn( target_sequence[0] );
	else apparatus->HorizontalTargetOn( target_sequence[0] );
		
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );

	// Step through the list of targets.
	for ( int target = 1; target < n_targets; target++ ) {
		
		// Light up the next target.
		apparatus->TargetsOff();
		if ( direction == VERTICAL ) apparatus->VerticalTargetOn( target_sequence[ target ] );
		else apparatus->HorizontalTargetOn( target_sequence[ target ] ); 


		// Make a beep. Here we test the tones and volumes.
		apparatus->SoundOn( target, target );
		apparatus->Wait( beepTime );
		apparatus->SoundOff();
		
		// Allow a fixed time to reach the target.
		// Takes into account the duration of the beep.
		apparatus->Wait( movementTime - beepTime );
		
	}
	
	// Mark the ending point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );

	// Collect one final second of data.
	apparatus->Wait( baselineTime );
	
	// Let the subject know that they are done.
	BlinkAll( apparatus );

	// Stop collecting data.
	apparatus->StopAcquisition();
	
	// Save the data.
	apparatus->SaveAcquisition( "TRGT" );
	
	// Check the quality of the data.
	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	if ( direction == VERTICAL ) status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, apparatus->jVector, NULL );
	else status = apparatus->CheckMovementAmplitude( targetedMinMovementExtent, targetedMaxMovementExtent, apparatus->kVector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( NORMAL_EXIT );
	
}


/*********************************************************************************/

int RunOscillations( DexApparatus *apparatus ) {
	
	int status = 0;
	
	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( 
		"Install the DEX Target Frame in the %s Position.\nPlace the Target Bar in the %s position.\nPlace the tapping surfaces in the %s position.\n\nPress <OK> when ready.",
		PostureString[PostureSeated], TargetBarString[TargetBarRight], TappingSurfaceString[TappingFolded] );
	if ( status == ABORT_EXIT ) exit( status );

	// Verify that it is in the correct configuration, and if not, 
	//  give instructions to the subject about what to do.
	status = apparatus->SelectAndCheckConfiguration( PostureSeated, TargetBarRight, TappingFolded );
	if ( status == ABORT_EXIT ) exit( status );

	// Tell the subject which configuration should be used.
	status = apparatus->fWaitSubjectReady( "Move to the flashing target.\nWhen 2 new targets appear, make oscillating\nmovements at approx. 1 Hz. " );
	if ( status == ABORT_EXIT ) exit( status );

	// Light up the central target.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationCenterTarget );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( oscillationCenterTarget );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	
	// Start acquiring data.
	apparatus->StartAcquisition( oscillationMaxTrialTime );
	
	// Collect one second of data while holding at the starting position.
	apparatus->Wait( baselineTime );
	
	// Show the limits of the oscillation movement by lighting 2 targets.
	apparatus->TargetsOff();
	apparatus->TargetOn( oscillationLowerTarget );
	apparatus->TargetOn( oscillationUpperTarget );
	
	// Measure data during oscillations performed over a fixed duration.
	apparatus->Wait( oscillationTime );
	
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "OSCI" );
	
	// Check the quality of the data.
	status = apparatus->CheckOverrun( "Acquisition overrun. Request instructions from ground." );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );
	
	status = apparatus->CheckVisibility( cumulativeDropoutTimeLimit, continuousDropoutTimeLimit, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementAmplitude( oscillationMaxMovementExtent, oscillationMaxMovementExtent, apparatus->jVector, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Check that we got a reasonable amount of movement.
	status = apparatus->CheckMovementCycles( oscillationMinCycles, oscillationMaxCycles, apparatus->jVector, cycleHysteresis, NULL );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	exit_status = NORMAL_EXIT;
	
	return( exit_status );
	
}

/*********************************************************************************/

int RunCollisions( DexApparatus *apparatus ) {
	
	int status = 0;
	
	Sleep( 1000 );
	apparatus->StartAcquisition( collisionMaxTrialTime );
	
	// Now wait until the subject gets to the target before moving on.
	status = apparatus->WaitUntilAtVerticalTarget( collisionInitialTarget );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( BEGIN_ANALYSIS );
	for ( int target = 0; target < collisionSequenceN; target++ ) {
		
		// Ready to start, so light up starting point target.
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		
		// Allow a fixed time to reach the starting point before we start blinking.
		apparatus->Wait( movementTime );
				
		apparatus->TargetsOff();
		if ( collisionSequence[target] ) {
			apparatus->VerticalTargetOn( collisionUpTarget );
			apparatus->MarkEvent( TRIGGER_MOVE_UP);
		}
		else {
			apparatus->VerticalTargetOn( collisionDownTarget );
			apparatus->MarkEvent( TRIGGER_MOVE_DOWN );
		}

		// Allow a fixed time to reach the target before we start blinking.
		apparatus->Wait( flashTime );
		apparatus->TargetsOff();
		apparatus->TargetOn( collisionInitialTarget );
		apparatus->Wait( movementTime - flashTime );

		
	}
	
	apparatus->TargetsOff();

	// Mark the starting point in the recording where post hoc tests should be applied.
	apparatus->MarkEvent( END_ANALYSIS );
	// Stop acquiring.
	apparatus->StopAcquisition();
	
	// Save the data and show it,
	apparatus->SaveAcquisition( "COLL" );

	// Check if trial was completed as instructed.
	status = apparatus->CheckMovementDirection( 1, 0.0, 1.0, 0.0, collisionMovementThreshold );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Check if collision forces were within range.
	status = apparatus->CheckForcePeaks( 1.0, 5.0, 1 );
	if ( status == IDABORT ) exit( ABORT_EXIT );
	// Same idea for acclerations.
	status = apparatus->CheckAccelerationPeaks( 1.0, 2.0, 1 );
	if ( status == IDABORT ) exit( ABORT_EXIT );

	// Indicate to the subject that they are done.
	status = apparatus->SignalNormalCompletion( "Block terminated normally." );
	if ( status == ABORT_EXIT ) exit( status );
	
	return( exit_status );
	
}
/*********************************************************************************/

int RunTargetCalibration( DexApparatus *apparatus ) {
	int exit_status;
	exit_status = apparatus->CalibrateTargets();
	return( exit_status );
}

/*********************************************************************************/

// Mesage handler for dialog box.

BOOL CALLBACK dexDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
        
		//      SetTimer( hDlg, 1, period * 1000, NULL );
		return TRUE;
		break;
		
    case WM_TIMER:
		return TRUE;
		break;
		
    case WM_CLOSE:
		EndDialog(hDlg, LOWORD(wParam));
		exit( 0 );
		return TRUE;
		break;
		
    case WM_COMMAND:
		switch ( LOWORD( wParam ) ) {
		case IDCANCEL:
			EndDialog(hDlg, LOWORD(wParam));
			exit( 0 );
			return TRUE;
			break;
		}
		
	}
    return FALSE;
}



/**************************************************************************************/

int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{
	
	DexApparatus *apparatus;
	DexTimer session_timer;
	
	TrackerType	tracker_type;
	TargetType	target_type;
	SoundType	sound_type;
	AdcType		adc_type;

	DexTracker	*tracker;
	DexTargets	*targets;
	DexSounds	*sounds;
	DexADC		*adc;	
	
	int protocol = TARGETED_PROTOCOL;
	char *script = "DexSampleScript.dex";

	int return_code;
	
	// Parse command line.
	
	// First, define the devices.
	if ( strstr( lpCmdLine, "-coda"   ) ) tracker_type = CODA_TRACKER;
	else if ( strstr( lpCmdLine, "-rt"     ) ) tracker_type = RTNET_TRACKER;
	else tracker_type = MOUSE_TRACKER;

	if ( strstr( lpCmdLine, "-glm"   ) ) {
		adc_type = GLM_ADC;
		target_type = GLM_TARGETS;
	}
	else {
		adc_type = MOUSE_ADC;
		target_type = SCREEN_TARGETS;
	}
	// Can override the target type even if the glm is present for analog.
	if ( strstr( lpCmdLine, "-screen"   ) ) target_type = SCREEN_TARGETS;

	if ( strstr( lpCmdLine, "-blaster"   ) ) sound_type = SOUNDBLASTER_SOUNDS;
	else sound_type = SCREEN_SOUNDS;

	// Now specify what protocol to run.

	if ( strstr( lpCmdLine, "-osc"    ) ) protocol = OSCILLATION_PROTOCOL;
	if ( strstr( lpCmdLine, "-coll"   ) ) protocol = COLLISION_PROTOCOL;
	if ( strstr( lpCmdLine, "-friction"   ) ) protocol = FRICTION_PROTOCOL;
	if ( strstr( lpCmdLine, "-script" ) ) protocol = RUN_SCRIPT;
	if ( strstr( lpCmdLine, "-calib"  ) ) protocol = CALIBRATE_TARGETS;
	if ( strstr( lpCmdLine, "-install"  ) ) protocol = INSTALL_PROCEDURE;
	
	switch ( tracker_type ) {

	case MOUSE_TRACKER:

		dlg = CreateDialog(hInstance, (LPCSTR)IDD_CONFIG, HWND_DESKTOP, dexDlgCallback );
		CheckRadioButton( dlg, IDC_SEATED, IDC_SUPINE, IDC_SEATED ); 
		CheckRadioButton( dlg, IDC_LEFT, IDC_RIGHT, IDC_LEFT ); 
		CheckRadioButton( dlg, IDC_HORIZ, IDC_VERT, IDC_VERT );
		CheckRadioButton( dlg, IDC_FOLDED, IDC_EXTENDED, IDC_FOLDED );
		CheckDlgButton( dlg, IDC_CODA_ALIGNED, true );
		CheckDlgButton( dlg, IDC_CODA_POSITIONED, true );
		ShowWindow( dlg, SW_SHOW );

		tracker = new DexMouseTracker( dlg );
		break;

	case CODA_TRACKER:
		tracker = new DexCodaTracker();
		break;

	case RTNET_TRACKER:
		tracker = new DexRTnetTracker();
		break;

	default:
		MessageBox( NULL, "Unkown tracker type.", "Error", MB_OK );

	}
		
	switch ( target_type ) {

	case GLM_TARGETS:
		targets = new DexNiDaqTargets(); // GLM
		break;

	case SCREEN_TARGETS:
		targets = new DexScreenTargets(); 
		break;

	default:
		MessageBox( NULL, "Unknown target type.", "Error", MB_OK );

	}

	switch ( sound_type ) {

	case SOUNDBLASTER_SOUNDS:
		sounds = new DexScreenSounds(); // Soundblaster sounds not yet implemented.
		break;

	case SCREEN_SOUNDS:
		sounds = new DexScreenSounds(); 
		break;

	default:
		MessageBox( NULL, "Unknown sound type.", "Error", MB_OK );

	}
	
	switch ( adc_type ) {

	case GLM_ADC:
		adc = new DexNiDaqADC(); // GLM targets not yet implemented.
		break;

	case MOUSE_ADC:
		adc = new DexMouseADC( dlg, GLM_CHANNELS ); 
		break;

	default:
		MessageBox( NULL, "Unknown adc type.", "Error", MB_OK );

	}

	// Create an apparatus by piecing together the components.
	apparatus = new DexApparatus( tracker, targets, sounds, adc );
	apparatus->Initialize();

	// Send information about the actual configuration to the ground.
	apparatus->SignalConfiguration();
	
	/*
	* Keep track of the elapsed time from the start of the first trial.
	*/
	DexTimerStart( session_timer );

	init_plots();

	// Create a dialog box locally that we can display while the data is being saved.
	saving_dlg = CreateDialog(hInstance, (LPCSTR)IDD_SAVING, HWND_DESKTOP, dexDlgCallback );
	ShowWindow( saving_dlg, SW_HIDE );
	
	/*
	* Run one of the protocols.
	*/
	
	switch ( protocol ) {

	case CALIBRATE_TARGETS:
		while ( RETRY_EXIT == RunTargetCalibration( apparatus ) );
		break;
		
	case RUN_SCRIPT:
		while ( RETRY_EXIT == RunScript( apparatus, script ) );
		break;
		
	case OSCILLATION_PROTOCOL:
		while ( RETRY_EXIT == RunOscillations( apparatus ) );
		break;
		
	case COLLISION_PROTOCOL:
		do {
			return_code = RunTransducerOffsetCompensation( apparatus );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		do {
			return_code = RunCollisions( apparatus );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		break;

	case FRICTION_PROTOCOL:
		do {
			return_code = RunTransducerOffsetCompensation( apparatus );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		do {
			return_code = RunFrictionMeasurement( apparatus );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );

		plot_data( apparatus );

		break;


	case TARGETED_PROTOCOL:

		do {
			return_code = RunTransducerOffsetCompensation( apparatus );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );

		do {
			return_code = RunTargeted( apparatus, VERTICAL, targetSequence, targetSequenceN );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		plot_data( apparatus );

		do {
			return_code = RunTargeted( apparatus, HORIZONTAL, targetSequence, targetSequenceN );
		} while ( return_code == RETRY_EXIT );
		if ( return_code == ABORT_EXIT ) return( ABORT_EXIT );
		plot_data( apparatus );

		break;

	case INSTALL_PROCEDURE:
		while ( RETRY_EXIT == RunInstall( apparatus ) );
		break;

		
	}
	
	apparatus->Quit();
	return 0;
}



