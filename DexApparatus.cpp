/*********************************************************************************/
/*                                                                               */
/*                                   DexApparatus.cpp                            */
/*                                                                               */
/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

// ATI Force/Torque Sensor Library
#include <ATIDAQ\ftconfig.h>

#include <fMessageBox.h>
#include <VectorsMixin.h>
#include <DexTimers.h>

// The dialog boxes are defined at the executable level.
#include "..\DexSimulatorApp\resource.h"

#include "DexMonitorServer.h"
#include "Dexterous.h"
#include "DexTargets.h"
#include "DexTracker.h"
#include "DexADC.H"
#include "DexSounds.h"
#include "DexApparatus.h"

#include "DexSimulatorGUI.h"

#define DEX_SIMULATOR_UPDATE_PERIOD 0.1

/***************************************************************************/

const int DexApparatus::negativeBoxMarker = DEX_NEGATIVE_BOX_MARKER;
const int DexApparatus::positiveBoxMarker = DEX_POSITIVE_BOX_MARKER;
const int DexApparatus::negativeBarMarker = DEX_NEGATIVE_BAR_MARKER;
const int DexApparatus::positiveBarMarker = DEX_POSITIVE_BAR_MARKER;

DexApparatus::DexApparatus( void ) {

	this->tracker = NULL;
	this->targets = NULL;
	this->sounds = NULL;
	this->adc = NULL;

}

DexApparatus::DexApparatus( DexTracker  *tracker,
						    DexTargets  *targets,
							DexSounds	*sounds,
							DexADC		*adc,
							DexMonitorServer *monitor,
							HWND		workspace_dlg,
							HWND		mass_dlg
						) {

	this->tracker = tracker;
	this->targets = targets;
	this->sounds = sounds;
	this->adc = adc;
	this->monitor = monitor;

	this->mass_dlg = mass_dlg;
	this->workspace_dlg = workspace_dlg;

	update_count = 0;
	update_period = DEX_SIMULATOR_UPDATE_PERIOD;
	DexTimerSet( update_timer, update_period );

}

/***************************************************************************/

void DexApparatus::Initialize( void ) {
  
	// Initialize the hardware.
	targets->Initialize();
	sounds->Initialize();
	tracker->Initialize();
	adc->Initialize();

	nVerticalTargets = targets->nVerticalTargets;
	nHorizontalTargets = targets->nHorizontalTargets;
	nTargets = nVerticalTargets + nHorizontalTargets;

	// Create bit masks for the horizontal and vertical targets.
	// If you change something here, don't forget to change it in the derived classes as well.
	verticalTargetMask = 0;
	for ( int i = 0; i < nVerticalTargets; i++ ) verticalTargetMask = ( verticalTargetMask << 1 ) + 1;
	horizontalTargetMask = 0;
	for ( i = 0; i < nHorizontalTargets; i++ ) horizontalTargetMask = ( horizontalTargetMask << 1 ) + 1;
	horizontalTargetMask = horizontalTargetMask << nVerticalTargets;

	nTones = sounds->nTones;

	nCodas = tracker->nCodas;
	nMarkers = tracker->nMarkers;

	nChannels = adc->nChannels;

	nForceTransducers = N_FORCE_TRANSDUCERS;
	nGauges = N_GAUGES;
	nSamplesForAverage = N_SAMPLES_FOR_AVERAGE;

	// Load the most recently defined target positions.
	LoadTargetPositions();
	// Load the calibration for each of the force transducers.
	InitForceTransducers();
 
	// Initialize the list of events.
	nEvents = 0;

	// Open a file to store the positions and orientations that were computed from
	// the individual marker positions. This allows us
	// to compare with the position and orientation used by DexMouseTracker
	// to compute the marker positions. It is for debugging purposes.
	char *filename = "DexApparatusDebug.mrk";

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fMessageBox( MB_OK, "DexApparatus", "Error openning file for write:\n %s", filename );
		exit( -1 );
	}
	fprintf( fp, "Time\tPx\tPy\tPz\tQx\tQy\tQz\tQm\n" );

	// Start with all the targets off.
	TargetsOff();
	// Make sure that the sound is off.
	SoundOff();

	// Send information about the actual configuration to the ground.
	SignalConfiguration();
	
}

void DexApparatus::Quit( void ) {

	fclose( fp );

	ReleaseForceTransducers();
	tracker->Quit();
	monitor->Quit();
	targets->Quit();
	sounds->Quit();
	adc->Quit();

}

/***************************************************************************/

// Need to know where each target LED is to implement WaitAtTarget().
// This routine should be called at the beginning of each trial,
// perhaps as part of CheckTrackerAlignment();

void DexApparatus::SetTargetPositions( void ) {

	int trg;

	// Read the stored locations back from disk. 
	// We assume that they were measured when the target frame was at the origin in 
	// the upright position, i.e. just after an alignment.
	
	LoadTargetPositions();

	// Get the current position and orientation of the target frame.
	// The target positions will be updated to correspond to the
	//  positions of the targets in this configuration.

	Vector3		offset;
	Quaternion	rotation;

	GetTargetFramePosition( offset, rotation );
	for ( trg = 0; trg < nTargets; trg++ ) {
		Vector3	hold;
		RotateVector( hold, rotation, targetPosition[trg] );
		AddVectors( targetPosition[trg], hold, offset );
	}

	// The vertical bar might be in either position (left or right).
	// That was OK for the calculation of the orientation,
	// but now we need to make sure that the targets on the vertical 
	// are where the vertical bar is.

	CodaFrame	frame;
	Vector3	bar_position;
	Vector3 target_position;
	Vector3 shift;

	// Get the current marker positions.
	tracker->GetCurrentMarkerFrame( frame );

	// Compute the midpoint between the two reference markers on the bar.
	AddVectors( bar_position, frame.marker[negativeBarMarker].position, frame.marker[positiveBarMarker].position );
	ScaleVector( bar_position, bar_position, 0.5 );

	// Compute the midpoint between the top and bottom target, in the new reference frame.
	AddVectors( target_position, targetPosition[0], targetPosition[nVerticalTargets - 1] );
	ScaleVector( target_position, target_position, 0.5 );

	// Compute the offset that will bring the two midpoints together.
	SubtractVectors( offset, bar_position, target_position );

	// Now compute an additional offset that corrects the offset of the bar markers 
	// in front of the targets LEDs and that places the target position to the right of the bar,
	// all of this the current reference frame of the target frame.
//	RotateVector( shift, rotation, BarMarkersToTargets );
//	AddVectors( offset, offset, shift );

	// Shift the target positions to match the position of the target bar.
	// Only the vertical targets move.
	for ( trg = 0; trg < nVerticalTargets; trg++ ) {
		AddVectors( targetPosition[trg], targetPosition[trg], offset );
	}
}

// Load the target positions that were acquired by CalibrateTargets().

void DexApparatus::LoadTargetPositions( char *filename ) {

	FILE *fp;
	targetPositionFilename = filename;
	int items;

	// Retrieve the expected positions of the targets in tracker coordinates.
	fp = fopen( filename, "r" );
	if ( fp ) {
		for ( int trg = 0; trg < nTargets; trg++ ) {
			items = fscanf( fp, "%lf, %lf, %lf\n", 
							&targetPosition[trg][X], 
							&targetPosition[trg][Y], 
							&targetPosition[trg][Z] );
		}
		fclose( fp );
	}
}

// Find the position of each target by having an operator place the maniplulandum
// next to each one.

int DexApparatus::CalibrateTargets( void ) {

	int status;
	Vector3		position;
	Quaternion	orientation;
	FILE *fp;

	for ( int trg = 0; trg < nTargets; trg++ ) {
		TargetOn( trg );
		status = WaitSubjectReady( "Place manipulandum at lighted target.\nPress <RETURN> to continue." );
		if ( status == ABORT_EXIT ) break;
		if ( !GetManipulandumPosition( position, orientation ) ) {
			status = SignalError( MB_ABORTRETRYIGNORE, "Manipulandum not visible." );
			if ( status == IDABORT ) return( ABORT_EXIT );
			if ( status == IDRETRY ) trg--;
		}
		else {
			CopyVector( targetPosition[trg], position );
		}
	}

	fp = fopen( targetPositionFilename, "w" );
	if ( !fp ) {
		fSignalError( MB_OK, "Error writing to %s.\nAborting.", targetPositionFilename );
		return( ABORT_EXIT );
	}
	for ( trg = 0; trg < nTargets; trg++ ) {
		fprintf( fp, "%f, %f, %f\n", targetPosition[trg][X], targetPosition[trg][Y], targetPosition[trg][Z] );
	}
	fclose( fp );
	return( NORMAL_EXIT );
}

/***************************************************************************/

int DexApparatus::CheckTrackerFieldOfView( int unit, unsigned long marker_mask, 
										   float min_x, float max_x,
										   float min_y, float max_y,
										   float min_z, float max_z, const char *msg ) {

	CodaFrame frame;
	int mrk;
	unsigned long bit;

	char info[10240] = "", marker_status[64];

	while ( 1) {

		bool out = false;
		// May need to wake up MDB, so sample twice. Second one should be good.
		for ( int i = 1; i < 2; i++ ) tracker->GetCurrentMarkerFrameIntrinsic( frame, unit );
		sprintf( info, "\n\nUnit: %d\n", unit );
		for ( mrk = 0, bit = 0x01; mrk < nMarkers; mrk++, bit = bit << 1 ) {
			if ( marker_mask & bit ) {
				sprintf( marker_status, "\n  Marker %d: ", mrk );
				if ( !frame.marker[mrk].visibility ) {
					out = true;
					strcat( marker_status, "Out of View\n" );
				}
				else if ( frame.marker[mrk].position[X] < min_x || 
						  frame.marker[mrk].position[X] > max_x ||
						  frame.marker[mrk].position[Y] < min_y ||
						  frame.marker[mrk].position[Y] > max_y ||
						  frame.marker[mrk].position[Z] < min_z ||
						  frame.marker[mrk].position[Z] > max_z )  {
					out = true;
					strcat( marker_status, "Out of Range in " );

					if ( frame.marker[mrk].position[X] < min_x || 
						 frame.marker[mrk].position[X] > max_x ) strcat( marker_status, "X" );
					if ( frame.marker[mrk].position[Y] < min_y || 
						 frame.marker[mrk].position[Y] > max_y ) strcat( marker_status, "Y" );
					if ( frame.marker[mrk].position[Z] < min_z || 
						 frame.marker[mrk].position[Z] > max_z ) strcat( marker_status, "Z" );
				}
				else strcat( marker_status, "OK!" );
				strcat( info, marker_status );

			}
		}
		if ( !out ) return( NORMAL_EXIT );

		strcat( info, "\n\nNOTE: XYZ refers to intrinsic CODA bar axes." );
		// Report the error to the user.
		int response = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s%s", msg, info );
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );

	}
}

/***************************************************************************/

int DexApparatus::CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, const char *msg ) {

	CodaFrame	frame[2];

	Update();

	tracker->GetCurrentMarkerFrameUnit( frame[0], 0 );
	tracker->GetCurrentMarkerFrameUnit( frame[1], 1 );

	unsigned long bit;
	int mrk;

	int good = 0;
	double distance;
	Vector3 delta;

	for ( mrk = 0, bit = 0x01; mrk < nMarkers; mrk++, bit = bit << 1 ) {
		if ( bit & marker_mask ) {
			if ( frame[0].marker[mrk].visibility && frame[1].marker[mrk].visibility ) {
				SubtractVectors( delta, frame[0].marker[mrk].position, frame[1].marker[mrk].position );
				distance = VectorNorm( delta );
				if ( distance < tolerance ) good++;
			}
		}
	}

	if ( good < n_good ) {
		int response = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s", msg );
		if ( response == IDABORT ) {
			monitor->SendEvent( "Manual Abort from CheckTrackerAlignment()." );
			return( ABORT_EXIT );
		}
		else if ( response == IDIGNORE ) {
			monitor->SendEvent( "Ignore Error from SelectAndCheckConfiguration." );
			return( IGNORE_EXIT );
		}
		else {
			monitor->SendEvent( "Retry exit from SelectAndCheckConfiguration." );
			return( RETRY_EXIT );
		}
	}
	else return( NORMAL_EXIT );
}

/***************************************************************************/

int DexApparatus::CheckTrackerPlacement( int unit, 
											const Vector3 expected_pos, float p_tolerance,
											const Quaternion expected_ori, float o_tolerance,
											const char *msg ) {
	Vector3 pos;
	Quaternion ori;

	Vector3 delta_pos;
	double distance, angle;

	// Get the placement of the CODA unit, expressed as a position vector and a rotation quaternion.
	tracker->GetUnitPlacement( unit, pos, ori );

	// Compare with the expected values.
	SubtractVectors( delta_pos, pos, expected_pos );
	distance = VectorNorm( delta_pos );
	angle = ToDegrees( AngleBetween( ori, expected_ori ) );
	
	if ( distance > p_tolerance || abs( angle ) > o_tolerance ) {
		int response = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s\n  Position error: %f\n  Orientation error: %f", msg, distance, angle );
	
		if ( response == IDABORT ) {
			monitor->SendEvent( "Manual Abort from CheckTrackerPlacement()." );
			return( ABORT_EXIT );
		}
		else if ( response == IDIGNORE ) {
			monitor->SendEvent( "Ignore Error from CheckTrackerPlacement." );
			return( IGNORE_EXIT );
		}
		else {
			monitor->SendEvent( "Retry exit from CheckTrackerPlacement." );
			return( RETRY_EXIT );
		}
	}
	else return( NORMAL_EXIT );
}

/***************************************************************************/

int DexApparatus::PerformTrackerAlignment( const char *msg ) {
	int error_code;
	// Ask the tracker to perform the alignment.
	error_code = tracker->PerformAlignment( negativeBoxMarker, 
													negativeBoxMarker, positiveBoxMarker, 
													negativeBarMarker, positiveBarMarker );
	return( error_code );
}

/***************************************************************************/

// Compute the 3D position vector and 4D orientation quaternion of the manipulandum.
// Takes as input a pointer to a buffer full of one slice of marker data (marker_frame);
// Fills in the position vector pos and the orientation quaternion ori.
bool DexApparatus::ComputeManipulandumPosition( Vector3 pos, Quaternion ori, CodaFrame &marker_frame, Quaternion default_orientation ) {

	Vector3		selected_model[DEX_MAX_MARKERS];
	Vector3		selected_actual[DEX_MAX_MARKERS];

	bool		visible;

	// Select the visible output markers and the corresponding inputs.
	int n_markers = 0;
	for ( int i = 0; i < nManipulandumMarkers; i++ ) {
		int mrk = ManipulandumMarkerID[i];
		if ( marker_frame.marker[mrk].visibility ) {
			CopyVector( selected_model[n_markers], ManipulandumBody[i] );
			CopyVector( selected_actual[n_markers], marker_frame.marker[mrk].position );
			n_markers++;
		}
	}

	// ComputeRigidBodyPose() does it all!
	visible = ComputeRigidBodyPose( pos, ori, 
									selected_model, selected_actual, n_markers, 
									default_orientation );
	return( visible );

}

// Compute the 3D position of the target frame.
bool DexApparatus::ComputeTargetFramePosition( Vector3 pos, Quaternion ori, CodaFrame &frame  ) {

	Matrix3x3	box_and_bar;
	Matrix3x3	ortho;

	// All reference markers have to be visible to compute the position and orientation of the frame.
	if ( !frame.marker[negativeBoxMarker].visibility ||
		 !frame.marker[positiveBoxMarker].visibility ||
		 !frame.marker[negativeBarMarker].visibility ||
		 !frame.marker[positiveBarMarker].visibility ) 
	{
		return( false );
	}
	
	// Position of the frame is just the position of the origin marker on the box.
	CopyVector( pos, frame.marker[negativeBoxMarker].position );

	// Now compute the orientation of the box/bar combination.
	SubtractVectors( box_and_bar[0], frame.marker[positiveBoxMarker].position, frame.marker[negativeBoxMarker].position );
	SubtractVectors( box_and_bar[1], frame.marker[positiveBarMarker].position, frame.marker[negativeBarMarker].position );
	OrthonormalizeMatrix( ortho, box_and_bar );
	MatrixToQuaternion( ori, ortho );

	return( true );
}


// Get the current position and orientation of the manipulandum.
bool DexApparatus::GetManipulandumPosition( Vector3 pos, Quaternion ori, Quaternion default_orientation ) {

	CodaFrame marker_frame;
	bool visible;

	// Make sure that the tracker has had a chance to update before doing so.
	// This is probably only an issue for the simulated trackers.
	// Actually, this is probably no longer an issue, since I stopped 
	// storing the state of the markers in the tracker.
	// if ( int exit_status = tracker->Update() ) exit( exit_status );

	// Here we ask for the current position of the markers from the tracker.
	tracker->GetCurrentMarkerFrame( marker_frame );

	// Compute the position vector and orientation quaternion from the markers.
	visible = ComputeManipulandumPosition( pos, ori, marker_frame, default_orientation );

	// For testing purposes, output the computed position and orientation.
	fprintf( fp, "%f\t%d\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n",
		marker_frame.time, visible, pos[X], pos[Y], pos[Z],
		ori[X], ori[Y], ori[Z], ori[M] );

	return( visible );
}

// Get the current position and orientation of the target frame.
bool DexApparatus::GetTargetFramePosition( Vector3 pos, Quaternion ori ) {

	CodaFrame marker_frame;
	bool visible;

	// Here we ask for the current position of the markers from the tracker.
	tracker->GetCurrentMarkerFrame( marker_frame );

	// Compute the position vector and orientation quaternion from the markers.
	visible = ComputeTargetFramePosition( pos, ori, marker_frame );

	return( visible );
}


/***************************************************************************/

// 
// This method gets called periodically during wait cycles.
// It calls the update methods for each of the subsystems.
//

void DexApparatus::Update( void ) {
	
	bool			manipulandum_visible;
	Vector3			pos;
	Quaternion		ori;
	bool			acquisition_state;
	unsigned long	target_state;
	
	int exit_status;

	// Don't actually run the update more often than necessary.
	if ( !DexTimerTimeout( update_timer ) ) return;

	// Allow each of the components to update as needed.
	if ( exit_status = targets->Update() ) exit( exit_status );
	if ( exit_status = tracker->Update() ) exit( exit_status );
	if ( exit_status = sounds->Update() ) exit( exit_status );
	if ( exit_status = adc->Update() ) exit( exit_status );
	
	// Compute the current state of the manipulandum;
	// Note that I no longer cache these information.
	// It is more efficient for the other routines to call them as
	// necessary.
	manipulandum_visible = GetManipulandumPosition( pos, ori );
			
	// Send state information to the ground.
	acquisition_state = tracker->GetAcquisitionState();
	target_state = targets->GetTargetState();
	monitor->SendState( acquisition_state, target_state, manipulandum_visible, pos, ori );

	// Wait at least update_period before running this update again.
	DexTimerSet( update_timer, update_period );
	update_count++;

}

/***************************************************************************/
/*                                                                         */
/*                             Subject Communication                       */
/*                                                                         */
/***************************************************************************/

// These steps are implemented to communicate with the subject during the
// trials, i.e. when he or she is not looking at the screen.
// We flash the target LEDs in some special way to signal to the subject that
// there is a message on the screen.

int DexApparatus::SignalError( unsigned int mb_type, const char *picture, const char *message ) {
		
	DexTimer move_timer;

	for ( int blinks = 0; blinks < N_ERROR_BLINKS; blinks++ ) {
		
		SoundOn( 2, BEEP_VOLUME );
		targets->SetTargetState( 0x55555555 );
		DexTimerSet( move_timer, BLINK_PERIOD );
		while( !DexTimerTimeout( move_timer ) ) Update();
		SoundOn( 5, BEEP_VOLUME );
		targets->SetTargetState( ~0x55555555 );
		DexTimerSet( move_timer, BLINK_PERIOD );
		while( !DexTimerTimeout( move_timer ) ) Update();
		SoundOff();	
	}
	
	targets->SetTargetState( ~0x00000000 );
	DexTimerSet( move_timer, BLINK_PERIOD );
	while( !DexTimerTimeout( move_timer ) ) Update();
	
	monitor->SendEvent( message );
	return( IllustratedMessageBox( picture, message, "DEX Message", mb_type ) );
	
}

// fprintf version of the above.

int DexApparatus::fSignalError( unsigned int mb_type, const char *picture, const char *format, ... ) {

	va_list args;
	char message[10240];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);

	return( SignalError( mb_type, picture, message ) );

}

// This method is implemented using the primitive commands of DexApparatus,
// not by communicating with the hardware directly.
// It is not a command that will generate a single script command.
// Rather, by calling this command, the primitives required to generate the 
// behavior will be written to the script.
// Thus, the Dex interpreter does not have to handle this command directly.

int DexApparatus::SignalNormalCompletion( const char *message ) {
		
	int status;

	Comment( "Signal Normal Completion." );
	for ( int blinks = 0; blinks < N_NORMAL_BLINKS; blinks++ ) {
		
		SoundOff();
		TargetsOff();
		Wait( BLINK_PERIOD );
		
		SoundOn( BEEP_TONE, BEEP_VOLUME );
		SetTargetState( ~0 );
		Wait( BLINK_PERIOD );
		
	}
	SoundOff();
	
	status = WaitSubjectReady( "Pictures\\info.bmp", message );
	if ( status == NORMAL_EXIT ) SignalEvent( "Normal Completion." );
	return( status );
	
}

// fprintf version of the above.

int DexApparatus::fSignalNormalCompletion( const char* format, ... ) {
	
	va_list args;
	char message[10240];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);

	return( SignalNormalCompletion( message ) );

}

// 
// Wait until the subject signals that he or she is ready to continue.
//

int DexApparatus::WaitSubjectReady( const char *picture, const char *message ) {
	
	Update();
	monitor->SendEvent( "WaitSubjectReady - %s", message );

	// There is a problem here. Update should be called while waiting for the subject's
	// response. Update() could perhaps be run in a thread.
	// But in the real system, the subsystems will presumably run in background 
	// threads, so this is not an issue.
//	int response = MessageBox( NULL, message, "DEX", MB_OKCANCEL | MB_ICONQUESTION );
	int response = IllustratedMessageBox( picture, message, "DEX", MB_OKCANCEL );
	Update();
	if ( response == IDCANCEL ) {
		monitor->SendEvent( "Manual Abort from WaitSubjectReady." );
		return( ABORT_EXIT );
	}
	
	return( NORMAL_EXIT );
	
}

// fprintf version of the above.

int DexApparatus::fWaitSubjectReady( const char *picture, const char* format, ... ) {
	
	va_list args;
	char message[10240];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);

	return( WaitSubjectReady( picture, message ) );

}

/***************************************************************************/
/*                                                                         */
/*                              Event Logging                              */
/*                                                                         */
/***************************************************************************/

// Log and send configuration information to the ground.

void DexApparatus::SignalConfiguration( void ) {
	
	int		n_codas, n_targets;
	n_codas = tracker->GetNumberOfCodas();
	n_targets = targets->nTargets;
	
	monitor->SendConfiguration( 
		n_codas, n_targets, 
		Posture(), 
		BarPosition(), 
		TappingDeployment() );
	
	monitor->SendEvent( "Current configuration: [%d %d %d]",
		Posture(), 
		BarPosition(), 
		TappingDeployment() );
	
	
}

// Log and signal an event to the ground.

void DexApparatus::SignalEvent( const char *msg ) {
	monitor->SendEvent( msg );
}

// Does nothing in execution mode.
// Will be replaced by the compiler to add comments in the script files.
void DexApparatus::Comment( const char *txt ) {}

// Log events locally, for use by post hoc tests.

void DexApparatus::ClearEventLog( void ) {
	nEvents = 0.0;
}

void DexApparatus::MarkEvent( int event, unsigned long param ) {
	eventList[nEvents].event = event;
	eventList[nEvents].param = param;
	eventList[nEvents].time = DexTimerElapsedTime( trialTimer );
	if ( nEvents < DEX_MAX_EVENTS ) nEvents++;
}

void DexApparatus::MarkTargetEvent( unsigned int bits ) {
	MarkEvent( TARGET_EVENT, bits );
}

void DexApparatus::MarkSoundEvent( int tone, int volume ) {
	MarkEvent( SOUND_EVENT, ( volume << 4 | tone ) );
}


// Calculate the approximate marker frame where the event occured.

int DexApparatus::TimeToFrame( float elapsed_time ) {
	int frame = (int) floor( elapsed_time / tracker->samplePeriod );
	// Make sure that it is a valid frame.
	if (frame < 0) frame = 0;
	if (frame >= nAcqFrames ) frame = nAcqFrames - 1;
	return( frame );
}

// Calculate the approximate analog sample where the event occured.

int DexApparatus::TimeToSample( float elapsed_time ) {
	int sample = (int) floor( elapsed_time / adc->samplePeriod );
	// Make sure that it is a valid frame.
	if (sample < 0) sample = 0;
	if (sample >= nAcqSamples ) sample = nAcqSamples - 1;
	return( sample );
}

// Find the events that determine the interval of analysis,
// based on the event markers BEGIN_ANALYSIS and END_ANAYLYSIS.

void DexApparatus::FindAnalysisEventRange( int &first, int &last ) {

	// Search backwards for the starting event. If we don't find one, start at 0;
	for ( first = nEvents - 1; first > 0; first-- ) {
		if ( eventList[first].event == BEGIN_ANALYSIS ) break;
	}
	// Search forward for the ending event. If we dont find it, take the last event.
	for ( last = first; last < nEvents - 1; last++ ) {
		if ( eventList[last].event == END_ANALYSIS ) break;
	}
}

// Find the corresponding marker frames.

void DexApparatus::FindAnalysisFrameRange( int &first, int &last ) {

	int first_event, last_event;

	FindAnalysisEventRange( first_event, last_event );
	first = TimeToFrame( eventList[first_event].time );
	last = TimeToFrame( eventList[last_event].time );

}

/***************************************************************************/

// Show the status to the subject.

void DexApparatus::ShowStatus ( const char *message ) {
	SetDlgItemText( workspace_dlg, IDC_STATUS_TEXT, message );
	// If we are updating the status, it is usually an interesting break point in the procedure.
	// So we automatically insert a comment in the script file to make it easy to find.
	Comment( message );
	// Tell the ground the same information.
	SignalEvent( message );
}

void DexApparatus::HideStatus ( void ) {
	ShowStatus( "" );
}


/*********************************************************************************/

// Compute the position of the vertical target bar with respect to the box.
// This algorithm should work no matter what the reference frame.
DexTargetBarConfiguration DexApparatus::BarPosition( void ) {

	CodaFrame	frame;
	tracker->GetCurrentMarkerFrame( frame );

	Vector3	box, bar;
	double  box_width, projection;

	// ALL reference markers must be visible. 
	// It would probably be OK if we only had 1 bar marker,
	// but we are going to want to see all 4 for other reasons.
	if ( !frame.marker[negativeBoxMarker].visibility ||
		 !frame.marker[positiveBoxMarker].visibility ||
		 !frame.marker[negativeBarMarker].visibility ||
		 !frame.marker[positiveBarMarker].visibility ) 
	{
		return( TargetBarUnknown );
	}

	SubtractVectors( box, 
					frame.marker[positiveBoxMarker].position, 
					frame.marker[negativeBoxMarker].position );
	box_width = VectorNorm( box );
	NormalizeVector( box );
	SubtractVectors( bar, 
					frame.marker[positiveBarMarker].position, 
					frame.marker[negativeBoxMarker].position );
	projection = DotProduct( bar, box );
	if ( projection > box_width / 2.0 ) return( TargetBarLeft );
	else return( TargetBarRight );

}

// This algorithm works only if the alignment has been done with the box in the upright posture.

DexSubjectPosture DexApparatus::Posture( void ) {

	CodaFrame	frame;
	tracker->GetCurrentMarkerFrame( frame );

	// Box reference markers must be visible. 
	if ( !frame.marker[negativeBoxMarker].visibility ||
		 !frame.marker[positiveBoxMarker].visibility  ) 
	{
		return( PostureUnknown );
	}

	if ( frame.marker[negativeBoxMarker].position[X] > frame.marker[positiveBoxMarker].position[X] ) return( PostureSupine );
	else return( PostureSeated );
}

DexTappingSurfaceConfiguration DexApparatus::TappingDeployment( void ) {
	return( TappingUnknown );
}

/*********************************************************************************/

// SelectAndCheckConfiguration compares the desired configuration to the actual 
//  configuration and prompts the subject to change until the desired configuration is achieved.
// User can specify TargetBarAny, PostureAny and/or TappingAny to ignore the test.

int DexApparatus::SelectAndCheckConfiguration( int posture, int bar_position, int tapping ) {
	
	int pass = 0;
	char *picture = 0;
	
	// Current simulated configuration.
	int current_posture;
	int current_bar_position;
	int current_tapping;
	
	// Record that we started this step.
	monitor->SendEvent( "SelectAndCheckConfiguration - Desired: [%d %d %d].", posture, bar_position, tapping );
	
	while ( 1 ) {
		
		// Signal to the ground what is the current configuration.
		SignalConfiguration();

		// Compute the target positions in that configuration.
		SetTargetPositions();
		
		// Check what is the current configuration to be compared with the desired.
		// Super classes implement tests to detect the target frame orientation and
		// the position of the vertical target bar.
		current_posture = Posture();
		current_bar_position = BarPosition();

		// The real DEX hardware cannot detect the deployment of the tapping surfaces.
		// I keep it here just for show. 
		current_tapping = TappingDeployment();
		
		if (  ( posture == DONT_CARE || posture == PostureIndifferent || posture == current_posture ) &&
			  ( bar_position == DONT_CARE || bar_position == TargetBarIndifferent || bar_position == current_bar_position ) && 
			  ( tapping == DONT_CARE || tapping == current_tapping ) 
		   ) {
			SignalEvent( "Successful configuration check." );
			return( NORMAL_EXIT );
		}

		// Select a picture file depending on the desired configuration.
		if ( posture == PostureSupine ) {
			if ( bar_position == TargetBarLeft ) picture = "Pictures\\SupineAside.bmp";
			else picture = "Pictures\\SupineInUse.bmp";
		}
		else {
			if ( bar_position == TargetBarLeft ) picture = "Pictures\\SitAside.bmp";
			else picture = "Pictures\\SitInUse.bmp";
		}

		
		// Signal to the subject that the configuration is currently not correct.
		// Allow them to retry to achieve the desired configuration, to ignore and move on, or to abort the session.
		int response = fSignalError(  MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, picture,
			"Change configuration to:\n  Subject Posture: %s\n  Target Bar: %s\nPress <Retry> when ready.", 
			PostureString[posture], TargetBarString[bar_position] );
		if ( response == IDABORT ) {
			SignalEvent(  "Manual Abort from SelectAndCheckConfiguration." );
			return( ABORT_EXIT );
		}
		if ( response == IDIGNORE ) {
			SignalEvent(  "Ignore Error from SelectAndCheckConfiguration." );
			return( IGNORE_EXIT );
		}
		// If we get here, it's a retry. Loop again to check if the change was successful.
		
	}
	
	// Should never get here.
	
}

int DexApparatus::SelectAndCheckMass( int mass ) {

	char *cradle;
	int  answer;

	if ( mass == MassIndifferent ) return( NORMAL_EXIT );

	do {

		if ( mass == MassNone && ( IsDlgButtonChecked( mass_dlg, IDC_MASS1M ) || IsDlgButtonChecked( mass_dlg, IDC_MASS1M ) || IsDlgButtonChecked( mass_dlg, IDC_MASS1M ) ) ) {
//			answer =  fMessageBox( MB_ABORTRETRYIGNORE, "DexApparatus", "Place manipulandum weight in empty cradle.\n" );
			answer =  fIllustratedMessageBox( MB_ABORTRETRYIGNORE, NULL, "DexApparatus", "Place manipulandum in empty cradle.\n" );
		}
		else {

			switch ( mass ) {

			case MassSmall:
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS1M ) ) return( NORMAL_EXIT );
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS1A ) ) cradle = "A";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS1B ) ) cradle = "B";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS1C ) ) cradle = "C";
				break;

			case MassMedium:
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS2M ) ) return( NORMAL_EXIT );
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS2A ) ) cradle = "A";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS2B ) ) cradle = "B";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS2C ) ) cradle = "C";
				break;

			case MassLarge:
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS3M ) ) return( NORMAL_EXIT );
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS3A ) ) cradle = "A";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS3B ) ) cradle = "B";
				if ( IsDlgButtonChecked( mass_dlg, IDC_MASS3C ) ) cradle = "C";
				break;

			}

//			answer = fMessageBox( MB_ABORTRETRYIGNORE, "DexApparatus", "Take manipulandum weight from cradle %s.\nPress RETRY when ready.", cradle );
			answer = fIllustratedMessageBox( MB_ABORTRETRYIGNORE, NULL, "DexApparatus", "Take the mass with the manipulandum \n from cradle %s.\nPress RETRY when ready.", cradle );

		}
	} while ( answer == IDRETRY );

	if ( answer == IDIGNORE ) return( IGNORE_EXIT );
	else return( ABORT_EXIT );

}


/***************************************************************************/
/*                                                                         */
/*                                 Wait Methods		                       */
/*                                                                         */
/***************************************************************************/

//
// Wait a fixed time, but update the manipulandum state while waiting.
//
void DexApparatus::Wait( double duration ) {
	
	DexTimer wait_timer;
	DexTimerSet( wait_timer, duration );
	while( !DexTimerTimeout( wait_timer ) ) {
		Update(); // This does the updating.
	}

}

/***************************************************************************/

//
// Wait until the manipulandum is placed at a target position.
//
int DexApparatus::WaitUntilAtTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {
	
	// These are objects of my own making, based on the Windows clock.
	DexTimer blink_timer;	
	DexTimer hold_timer;
	DexTimer timeout_timer;

	// Holds the current position and orientation of the manipulandum.
	bool		manipulandum_visible;
	Vector3		manipulandum_position;
	Quaternion	manipulandum_orientation;
	
	// These will hold the error in position and orientation of the manipulandum with respect
	//  to the specified desired target position and the specified orientation.
	Vector3 difference;
	float	misorientation;

	bool led_state = 0;
	int  mb_reply;
	
	// Log that the method has started.
	SignalEvent(  "WaitUntilAtTarget - Start." );
	DexTimerSet( timeout_timer, timeout );
	while ( 1 ) {
		
		led_state = 0;
		TargetsOff();
		TargetOn( target_id );
		DexTimerSet( blink_timer, waitBlinkPeriod );
		do {

			if ( DexTimerTimeout( timeout_timer ) ) {
				
				// Timeout has been reached. Signal the error to the user.
				if ( !msg ) msg = "Time to reach target exceeded.\n Is the manipulandum visible?\n Is the manipulandum upright?\n";
				mb_reply = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, 
					"%s\n  Target ID: %d\n  Max time: %.2f\n  Manipulandum Visible: %s\n Manipulandum Position: <%.3f %.3f %.3f>\n Position Error: <%.3f %.3f %.3f>\n Manipuladum orientation: <%.3f %.3f %.3f %.3f>\n  Orientation Error: %.0f degrees",
					msg, target_id,
					timeout, ( manipulandum_visible ? "yes" : "no" ), 
					manipulandum_position[X], manipulandum_position[Y],
					manipulandum_position[Z],
					difference[X],difference[Y],difference[Z],
					manipulandum_orientation[X], manipulandum_orientation[Y],
					manipulandum_orientation[Z], manipulandum_orientation[M],
					misorientation );
				// Exit, signalling that the subject wants to abort.
				if ( mb_reply == IDABORT ) {
					SignalEvent(  "Manual Abort from WaitUntilAtTarget." );
					return( ABORT_EXIT );
				}
				// Ignore the error and move on.
				if ( mb_reply == IDIGNORE ) {
					SignalEvent(  "Ignore Timeout from WaitUntilAtTarget." );
					return( IGNORE_EXIT );
				}
				
				// Try again for another timeout period. Thus, retry restarts the step, 
				// not the entire script.
				SignalEvent(  "Retry after Timeout from WaitUntilAtTarget." );
				DexTimerSet( timeout_timer, waitTimeLimit );
				
			}
			
			// This toggles the target LED on and off to attract attention.
			if ( DexTimerTimeout( blink_timer ) ) {
				TargetsOff();
				led_state = !led_state;
				if ( led_state ) TargetOn( target_id );
				DexTimerSet( blink_timer, waitBlinkPeriod );
			}
			
			// The apparatus class has an Update() method that must be called during any 
			// wait loop, in case any of the hardware devices need periodic updating.
			Update();

			// Get the current position and orientation of the manipulandum.
			manipulandum_visible = GetManipulandumPosition( manipulandum_position, manipulandum_orientation );
			
			// Compare the computed position and orientation to the desired position and orientation.
			misorientation = ToDegrees( AngleBetween( desired_orientation, manipulandum_orientation ) ) ;
			SubtractVectors( difference, targetPosition[target_id], manipulandum_position );
			
		} while ( 
			// Stay here until the manipulandum is at the target, or until timeout.
			// It is assumed that targetPosition[][] contains the position of each 
			//  target LED in the current tracker reference frame, and that the 
			//  manipulandum position is being updated as noted above.
			abs( difference[X] ) > position_tolerance[X] ||
			abs( difference[Y] ) > position_tolerance[Y] ||
			abs( difference[Z] ) > position_tolerance[Z] ||
			misorientation > orientation_tolerance ||
			!manipulandum_visible
			);

		// Target goes on steady if the manipulandum is in the zone.
		TargetsOff();
		TargetOn( target_id );
		// Set a timer to determine if the manipulandum is in the right position 
		// for a long enough time.
		DexTimerSet( hold_timer, hold_time );
		do {
			// If the manipulandum has been in the zone long enough, return with SUCCESS.
			if ( DexTimerTimeout( hold_timer ) ) {
				SignalEvent(  "WaitUntilAtTarget - Success." );
				return( NORMAL_EXIT );
			}
			// Continue to perform required updates during the wait period.
			Update();

			// Get the current position and orientation of the manipulandum.
			manipulandum_visible = GetManipulandumPosition( manipulandum_position, manipulandum_orientation );

			// Compare the computed position and orientation to the desired position and orientation.
			misorientation = ToDegrees( AngleBetween( desired_orientation, manipulandum_orientation ) ) ;
			SubtractVectors( difference, targetPosition[target_id], manipulandum_position );
			
		} while ( 
			abs( difference[X] ) <= position_tolerance[X] &&
			abs( difference[Y] ) <= position_tolerance[Y] &&
			abs( difference[Z] ) <= position_tolerance[Z] &&
			misorientation < orientation_tolerance &&
			manipulandum_visible
			);

		// If we get here, it means that we did not stay in the zone long enough.
		// Loop continuously until we succeed or until a timeout.
		
	}
	
	// Should never get here.
	return 0; 
	
}

// Convenience methods for my own use.

int DexApparatus::WaitUntilAtVerticalTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {
	return( WaitUntilAtTarget( target_id, desired_orientation, position_tolerance, orientation_tolerance, hold_time, timeout, msg ) );
}

int DexApparatus::WaitUntilAtHorizontalTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {
	return( WaitUntilAtTarget( nVerticalTargets + target_id, desired_orientation, position_tolerance, orientation_tolerance, hold_time, timeout, msg  ) );
}

/***************************************************************************/

//
// Wait until the grip is properly centered.
//
int DexApparatus::WaitCenteredGrip( float tolerance, float min_force, float timeout, char *msg  ) {
	
	// These are objects of my own making, based on the Windows clock.
	DexTimer timeout_timer;

	// Holds the current center of pressure.
	Vector3		cop[N_FORCE_TRANSDUCERS];
	double		cop_offset[N_FORCE_TRANSDUCERS];

	bool		centered;
	int			mb_reply;
	
	// Log that the method has started.
	monitor->SendEvent( "WaitCenteredGrip - Start." );
	DexTimerSet( timeout_timer, timeout );
		
	do {

		if ( DexTimerTimeout( timeout_timer ) ) {
			
			// Timeout has been reached. Signal the error to the user.
			if ( !msg ) msg = "Time to achieve centered grip exceeded.";
			mb_reply = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, 
				"%s\n Tolerance: %.1f mm\n Min Force: %.2f N\n Unit 0: %.1f %s\n Unit 1: %.1f %s\n", 
				msg, tolerance, min_force,
				cop_offset[0] * 1000.0, vstr( cop[0] ), cop_offset[1] * 1000.0, vstr( cop[1] )  );
			// Exit, signalling that the subject wants to abort.
			if ( mb_reply == IDABORT ) {
				monitor->SendEvent( "Manual Abort from WaitCenteredGrip." );
				return( ABORT_EXIT );
			}
			// Ignore the error and move on.
			if ( mb_reply == IDIGNORE ) {
				monitor->SendEvent( "Ignore Timeout from WaitCenteredGrip." );
				return( IGNORE_EXIT );
			}
			
			// Try again for another timeout period. Thus, retry restarts the step, 
			// not the entire script.
			monitor->SendEvent( "Retry after Timeout from WaitCenteredGrip." );
			DexTimerSet( timeout_timer, waitTimeLimit );
			
		}
		
		
		// Do whatever updates are needed during a wait cycle.
		Update();

		centered = true;
		// Check if each sensor has its CoP near enough to the center.
		// The test will also fail if there is not enough pressure, which
		// we assume to mean that the subject is not holding the manipulandum
		// properly in a pinch grip.
		for ( int unit = 0; unit < nForceTransducers; unit++ ) {
			cop_offset[unit] = GetCOP( cop[unit], unit, min_force );
			// A negative offset means that the normal force is below threshold.
			// The offset threshold is expressed in mm, but the COP is calculated in m.
			if ( cop_offset[unit] < 0 || cop_offset[unit] > tolerance / 1000.0 ) centered = false;
		}

	} while ( !centered );

	return( NORMAL_EXIT );
	
}
	
/***************************************************************************/

// Use the target LEDs to give feedback about the applied grip and load force.
// This sets up the mapping from the forces to the visible target LEDs.

void DexApparatus::MapForceToLED( float min_grip, float max_grip, float min_load, float max_load ) {

	// Map force ranges to visible LEDs.
	// Desired grip forces are to be positive. 
	// If they are negative, that means we don't care.
	if ( max_grip < 0 && min_grip < 0 ) {
		max_grip_led = -1;
		min_grip_led = -1;
		grip_to_led = 0.0;
		grip_offset = 0.0;
	}
	if ( max_grip < 0 ) {
		max_grip_led = -1;
		min_grip_led = nHorizontalTargets - 5;
		grip_to_led = (double) min_grip_led / min_grip;
		grip_offset = 0.0;
	}
	else if ( min_grip < 0 ) {
		min_grip_led = -1;
		max_grip_led = nHorizontalTargets - 5;
		grip_to_led = (double) max_grip_led / max_grip;
		grip_offset = 0.0;
	}
	else {
		// Map the allowable range to a set of LEDs.
		min_grip_led = nHorizontalTargets - 6;
		max_grip_led = nHorizontalTargets - 2;
		// The distance between the min and max allowable force defines the gain
		// factor between Newtons and LED steps.
		grip_to_led = (double) (max_grip_led - min_grip_led) / (max_grip - min_grip);
		// Compute how much force is represented by the LEDs below the lower one.
		// You need at least this much force to get the LED to move.
		grip_offset = min_grip - min_grip_led / grip_to_led;
	}

	// Map load force ranges to visible LEDs.
	// Desired grip forces are to be positive. 
	// If they are negative, that means we don't care.
	zero_load_led = nVerticalTargets - 9;
	if ( max_load < 0 && min_load < 0 ) {
		max_load_led = -1;
		min_load_led = -1;
		load_to_led = 0.0;
		load_offset = 0.0;
	}
	if ( max_load < 0 ) {
		max_load_led = -1;
		min_load_led = nVerticalTargets - 5;
		load_to_led = (double) min_load_led / min_load;
		load_offset = 0.0;
	}
	else if ( min_load < 0 ) {
		min_load_led = -1;
		max_load_led = nVerticalTargets - 5;
		load_to_led = (double) max_load_led / max_load;
		load_offset = 0.0;
	}
	else {
		// Map the allowable range to a set of LEDs.
		min_load_led = nVerticalTargets - 6;
		max_load_led = nVerticalTargets - 2;
		// The distance between the min and max allowable force defines the gain
		// factor between Newtons and LED steps.
		load_to_led = (double) (max_load_led - min_load_led) / (max_load - min_load);
		// Compute how much force is represented by the LEDs below the lower one.
		// You need at least this much force to get the LED to move.
		load_offset = min_load - min_load_led / load_to_led;
	}

	DexTimerSet( blink_timer, waitBlinkPeriod );
	blink = true;

}

// This routine runs the LEDs during the routines that feedback force info.

void DexApparatus::UpdateForceToLED( float grip, float load ) {

	int cursor;

	if ( DexTimerTimeout( blink_timer ) ) {
		blink = !blink;
		DexTimerSet( blink_timer, waitBlinkPeriod );
	}
	TargetsOff();

	// Show the desired range of grip forces.
	if ( min_grip_led >= 0 ) HorizontalTargetOn( min_grip_led );
	if ( max_grip_led >= 0 ) HorizontalTargetOn( max_grip_led );

	// Make a blinking target follow the measured grip force.
	cursor = floor( (grip - grip_offset) * grip_to_led );
	if ( cursor < 0 ) cursor = 0;
	if ( cursor >= nHorizontalTargets ) cursor = nHorizontalTargets - 1;
	if ( blink ) HorizontalTargetOn( cursor );
	else  HorizontalTargetOff( cursor );

	// Show the desired range of load forces.
	if ( min_load_led >= 0 ) VerticalTargetOn( min_load_led );
	if ( max_load_led >= 0 ) VerticalTargetOn( max_load_led );

	cursor = floor( (load - load_offset) * load_to_led );
	if ( cursor < 0 ) cursor = 0;
	if ( cursor >= nVerticalTargets ) cursor = nVerticalTargets - 1;
	if ( blink ) VerticalTargetOn( cursor );
	else  VerticalTargetOff( cursor );

	Update();

}

// 
// Wait until an appropriate combination of grip force and load force is achieved.
//

int DexApparatus::WaitDesiredForces( float min_grip, float max_grip, 
									 float min_load, float max_load, 
									 Vector3 direction,
									 float filter_constant,
									 float hold_time, float timeout, const char *msg ) {

	Vector3		force[N_FORCE_TRANSDUCERS], torque[N_FORCE_TRANSDUCERS];

	double		grip, load;
	Vector3		load_force;

	DexTimer	timeout_timer, hold_timer;

	int			unit;

	// Make sure that the direction vector is a unit vector.
	NormalizeVector( direction );

	// Set up the parameters of the visual feedback.
	MapForceToLED( min_grip, max_grip, min_load, max_load );

	DexTimerSet( timeout_timer, timeout );
	DexTimerSet( hold_timer, hold_time );
	while (1) {

		if ( DexTimerTimeout( timeout_timer ) ) {

			int mb_reply;
			
			// Timeout has been reached. Signal the error to the user.
			if ( !msg ) msg = "Time to achieve desired forces exceeded.";
			mb_reply = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s", msg );
			// Exit, signalling that the subject wants to abort.
			if ( mb_reply == IDABORT ) {
				monitor->SendEvent( "Manual Abort from WaitDesiredForce()." );
				return( ABORT_EXIT );
			}
			// Ignore the error and move on.
			if ( mb_reply == IDIGNORE ) {
				monitor->SendEvent( "Ignore Timeout from WaitDesiredForce()." );
				return( IGNORE_EXIT );
			}
			
			// Try again for another timeout period. Thus, retry restarts the step, 
			// not the entire script.
			monitor->SendEvent( "Retry after Timeout from WaitDesiredForces()." );
			DexTimerSet( timeout_timer, timeout );
			
		}

		for ( unit = 0; unit < N_FORCE_TRANSDUCERS; unit++ ) GetForceTorque( force[unit], torque[unit], unit ); 
		grip = ComputeGripForce( force[0], force[1] );
		ComputeLoadForce( load_force, force[0], force[1] );
		load = DotProduct( load_force, direction );

		// Filter the readings.
		grip = FilteredGrip( grip, filter_constant );
		load = FilteredLoad( load, filter_constant );

		// If either force is out of range, restart the timer that measures
		// how long we have been in range.
		if ( min_grip >= 0 && grip < min_grip ) DexTimerSet( hold_timer, hold_time );
		if ( max_grip >= 0 && grip > max_grip ) DexTimerSet( hold_timer, hold_time );
		if ( min_load >= 0 && load < min_load ) DexTimerSet( hold_timer, hold_time );
		if ( max_load >= 0 && load > max_load ) DexTimerSet( hold_timer, hold_time );

		// If this timer runs out, it means that we were in the range for the required
		// amount of time and can move on.
		if ( DexTimerTimeout( hold_timer ) ) break;

		UpdateForceToLED( grip, load );

	} 

	TargetsOff();
	Update();

	return( NORMAL_EXIT );
}

// 
// Wait until the fingers slip.
//

int DexApparatus::WaitSlip( float min_grip, float max_grip, 
							float min_load, float max_load, 
							Vector3 direction,
							float filter_constant,
							float slip_threshold, float timeout, const char *msg ) {

	Vector3		force[N_FORCE_TRANSDUCERS], torque[N_FORCE_TRANSDUCERS];
	Vector3		cop[N_FORCE_TRANSDUCERS], initial_cop[N_FORCE_TRANSDUCERS], delta_cop[N_FORCE_TRANSDUCERS];

	double		grip, load, delta[N_FORCE_TRANSDUCERS];
	Vector3		load_force;

	DexTimer	timeout_timer;

	int			unit;

	bool		slipped;

	// Make sure that the direction vector is a unit vector.
	NormalizeVector( direction );

	// Set up the parameters of the visual feedback.
	MapForceToLED( min_grip, max_grip, min_load, max_load );

	// Measure where the fingers are to start.
	for ( unit = 0; unit < N_FORCE_TRANSDUCERS; unit++ ) {
		GetForceTorque( force[unit], torque[unit], unit ); 
		ComputeCOP( initial_cop[unit], force[unit], torque[unit] );
	}

	DexTimerSet( timeout_timer, timeout );
	do {

		if ( DexTimerTimeout( timeout_timer ) ) {

			int mb_reply;
			
			// Timeout has been reached. Signal the error to the user.
			if ( !msg ) msg = "Time to achieve finger slip exceeded.";
			mb_reply = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s", msg );
			// Exit, signalling that the subject wants to abort.
			if ( mb_reply == IDABORT ) {
				monitor->SendEvent( "Manual Abort from WaitSlip()." );
				return( ABORT_EXIT );
			}
			// Ignore the error and move on.
			if ( mb_reply == IDIGNORE ) {
				monitor->SendEvent( "Ignore Timeout from WaitSlip()." );
				return( IGNORE_EXIT );
			}
			
			// Try again for another timeout period. Thus, retry restarts the step, 
			// not the entire script.
			monitor->SendEvent( "Retry after Timeout from WaitDesiredForces()." );
			DexTimerSet( timeout_timer, timeout );
			
		}

		slipped = true;
		for ( unit = 0; unit < N_FORCE_TRANSDUCERS; unit++ ) {
			GetForceTorque( force[unit], torque[unit], unit ); 
			if ( ComputeCOP( cop[unit], force[unit], torque[unit] ) > 0.0 ) {
				SubtractVectors( delta_cop[unit], cop[unit], initial_cop[unit] );
				delta[unit] = VectorNorm( delta_cop[unit] );
				if ( delta[unit] < slip_threshold / 1000.0 ) slipped = false;
			}
		}
		grip = ComputeGripForce( force[0], force[1] );
		ComputeLoadForce( load_force, force[0], force[1] );
		load = DotProduct( load_force, direction );

		// Filter the readings.
		grip = FilteredGrip( grip, filter_constant );
		load = FilteredLoad( load, filter_constant );

		UpdateForceToLED( grip, load );

	} while ( !slipped );

	TargetsOff();
	Update();

	return( NORMAL_EXIT );
}
/*********************************************************************************/
/*                                                                               */
/*                                Stimulus Control                               */
/*                                                                               */
/*********************************************************************************/

// Set the state of the target LEDs.

void DexApparatus::SetTargetStateInternal( unsigned long target_state ) {
	targets->SetTargetState( target_state );
}

void DexApparatus::SetTargetState( unsigned long target_state ) {
	SetTargetStateInternal( target_state );
	currentTargetState = target_state;
	MarkTargetEvent( target_state );
}

// The following are convenience methods for my own use.

void DexApparatus::TargetsOff( void ) {
	SetTargetState( 0x00000000L );
}

void DexApparatus::TargetOn( int id ) {
	SetTargetState( currentTargetState | ( 0x00000001L << id ) );
}

void DexApparatus::TargetOff( int id ) {
	SetTargetState( currentTargetState & ~( 0x00000001L << id ) );
}

void DexApparatus::VerticalTargetOn( int id ) {
	TargetOn( id );
}

void DexApparatus::HorizontalTargetOn( int id ) {
	TargetOn( nVerticalTargets + id );
}

void DexApparatus::VerticalTargetOff( int id ) {
	TargetOff( id );
}

void DexApparatus::HorizontalTargetOff( int id ) {
	TargetOff( nVerticalTargets + id );
}

int DexApparatus::DecodeTargetBits( unsigned long bits ) {
	for ( int i = 0; i < nTargets; i++ ) {
		if ( ( 0x01 << i ) & bits ) return( i );
	}
	return( TARGETS_OFF );
}

/*********************************************************************************/

// Initiate a constant tone. Will continue until stopped.
// This is the lowest level routine to be implemented in the DEX scripting language.
// This routine will get overlayed by the compiler.
void DexApparatus::SetSoundStateInternal( int tone, int volume ) {
	sounds->SetSoundState( tone, volume );
}

// This one notes the event as well, then calls the underlying routine.
void DexApparatus::SetSoundState( int tone, int volume ) {
	SetSoundStateInternal( tone, volume );
	MarkSoundEvent( tone, volume );
}

// The following are convenience methods for my use, built on SetSoundState().
void DexApparatus::SoundOn( int tone, int volume ) {
	SetSoundState( tone, volume );
}

// Stop all sounds. Do this by playing a sound at zero volume.
// This way, only SetSoundState() has to be implemented by the interpreter.

void DexApparatus::SoundOff( void ) {
	SetSoundState( 0, 0 );
}

// Play a brief tone.

void DexApparatus::Beep( int tone, int volume, float duration ) {
	SoundOn( tone, volume );
	Wait( duration );
	SoundOff();
}

/*********************************************************************************/
/*                                                                               */
/*                                 Data Acquisition                              */
/*                                                                               */
/*********************************************************************************/

// These are the acqusition routines according to my design. 
// TODO: Should be updated to the new specification where SaveAcqusition 
// does not exist.
void DexApparatus::StartAcquisition( const char *tag, float max_duration ) {

	// Store the tag that will be used in the filename.
	strncpy( filename_tag, tag, sizeof( filename_tag ) );

	// Reset the list of target and sound events.
	nEvents = 0;
	// Tell the tracker to start acquiring.
	tracker->StartAcquisition( max_duration );
	// And the ADC, too.
	// The real system worries about syncronizing. Here I do not.
	adc->StartAcquisition( max_duration );

	// Keep track of how long we have been acquiring.
	DexTimerSet( trialTimer, max_duration );
	// Tell the ground what we just did.
	monitor->SendEvent( "Acquisition started. Max duration: %f seconds.", max_duration );
	// Note the time of the acqisition start.
	MarkEvent( ACQUISITION_START );
}

int DexApparatus::StopAcquisition( const char *msg ) {

	int unit;

	MarkEvent( ACQUISITION_STOP );
	tracker->StopAcquisition();
	adc->StopAcquisition();
	ShowStatus( "Acquisition terminated.\nComputing kinematic values ..." );
	// Retrieve the marker data.
	for ( unit = 0; unit <= nCodas; unit++ ) {
		nAcqFrames = tracker->RetrieveMarkerFrames( acquiredPosition[unit], DEX_MAX_MARKER_FRAMES, unit );
	}
	// Compute the manipulandum positions.
	for ( int i = 0; i < nAcqFrames; i++ ) {
		Vector3		pos;
		Quaternion	ori;
		// Compute the manipulandum position and orientation at each time step.
		acquiredManipulandumState[i].visibility = 
			ComputeManipulandumPosition( pos, ori, acquiredPosition[0][i] );
		acquiredManipulandumState[i].time = acquiredPosition[0][i].time;
		CopyVector( acquiredManipulandumState[i].position, pos );
		CopyQuaternion( acquiredManipulandumState[i].orientation, ori );
	}
	// Send the marker recording by telemetry to the ground for monitoring.
	monitor->SendRecording( acquiredManipulandumState, nAcqFrames, INVISIBLE );

	// Retrieve the recorded analog data.
	nAcqSamples = adc->RetrieveAnalogSamples( acquiredAnalog, DEX_MAX_ANALOG_SAMPLES );

	// Save the data.
	SaveAcquisition( filename_tag );
		
	int status = CheckOverrun( msg );
	if ( status == ABORT_EXIT || status == RETRY_EXIT ) return( status );



#ifndef NOATI
	// Compute forces from analog data.
	for ( int smpl = 0; smpl < nAcqSamples; smpl++ ) {
		for ( int unit = 0; unit < N_FORCE_TRANSDUCERS; unit++ ) {
			ComputeForceTorque( acquiredForce[unit][smpl], acquiredTorque[unit][smpl], unit, acquiredAnalog[smpl] );
			ComputeCOP( acquiredCOP[unit][smpl], acquiredForce[unit][smpl], acquiredTorque[unit][smpl] );
		}
		acquiredGripForce[smpl] = ComputeGripForce( acquiredForce[0][smpl], acquiredForce[1][smpl] );
		acquiredLoadForceMagnitude[smpl] = 
			ComputePlanarLoadForce( acquiredLoadForce[smpl], acquiredForce[0][smpl], acquiredForce[1][smpl] );
		acquiredAcceleration[smpl][X] = acquiredAnalog[smpl].channel[lowAccAnalogChannel + X];
		acquiredAcceleration[smpl][Y] = acquiredAnalog[smpl].channel[lowAccAnalogChannel + Y];
		acquiredAcceleration[smpl][Z] = acquiredAnalog[smpl].channel[lowAccAnalogChannel + Z];
		acquiredHighAcceleration[smpl] = acquiredAnalog[smpl].channel[highAccAnalogChannel];
	}
#endif

	return( NORMAL_EXIT );

}
void DexApparatus::SaveAcquisition( const char *tag ) {
	
	FILE *fp;
	char fileroot[256], filename[512];

	int unit, frm, mrk, smpl, chan;
	
	// TODO: Automatically generate a file name.
	// Here we use the same name each time, but just add the tag.
	sprintf( fileroot, "DexSimulatorOutput.%s", tag );
	
	// Write a file with raw marker data.
	ShowStatus( "Writing marker data ..." );
	sprintf( filename, "%s.mrk", fileroot );
	fp = fopen( filename, "w" );
	fprintf( fp, "Sample\tTime" );
	for ( mrk = 0; mrk < nMarkers; mrk++ ) fprintf( fp, "\tM%02dV\tM%02dX\tM%02dY\tM%02dZ", mrk, mrk, mrk, mrk );
	for ( unit = 0; unit <= nCodas; unit++ ) {
		for ( mrk = 0; mrk < nMarkers; mrk++ ) {
			fprintf( fp, "\tU%1dM%02dV\tU%1dM%02dX\tU%1dM%02dY\tU%1dM%02dZ", unit, mrk, unit, mrk, unit, mrk, unit, mrk );
		}
	}
	fprintf( fp, "\n" );
	for ( frm = 0; frm < nAcqFrames; frm++ ) {
		fprintf( fp, "%d\t%.3f", frm, acquiredManipulandumState[frm].time ); 
		for ( unit = 0; unit <= nCodas; unit++ ) {
		for ( mrk = 0; mrk < nMarkers; mrk++ ) {
				fprintf( fp, "\t%d\t%f\t%f\t%f", 
					acquiredPosition[unit][frm].marker[mrk].visibility,
					acquiredPosition[unit][frm].marker[mrk].position[X],
					acquiredPosition[unit][frm].marker[mrk].position[Y],
					acquiredPosition[unit][frm].marker[mrk].position[Z] );
		}
		}
		fprintf( fp, "\n" );
	}
	fclose( fp );
	// Note that the file was written.
	monitor->SendEvent( "Data file written: %s", filename );
	
	// Write a file with the computed manipulandum position and orientation.
	ShowStatus( "Writing kinematic data ..." );
	sprintf( filename, "%s.mnp", fileroot );
	fp = fopen( filename, "w" );
	fprintf( fp, "Sample\tTime\tVisible\tPx\tPy\tPz\tQx\tQy\tQz\tQm\n" );
	for ( frm = 0; frm < nAcqFrames; frm++ ) {
		fprintf( fp, "%d\t%.3f\t%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", 
			frm, 
			acquiredManipulandumState[frm].time, 
			acquiredManipulandumState[frm].visibility,
			acquiredManipulandumState[frm].position[X], 
			acquiredManipulandumState[frm].position[Y], 
			acquiredManipulandumState[frm].position[Z],
			acquiredManipulandumState[frm].orientation[X], 
			acquiredManipulandumState[frm].orientation[Y], 
			acquiredManipulandumState[frm].orientation[Z],
			acquiredManipulandumState[frm].orientation[M] );
	}
	fclose( fp );
	// Note that the file was written.
	monitor->SendEvent( "Data file written: %s", filename );

	// Write a file with raw adc data.
	ShowStatus( "Writing ADC data ..." );
	sprintf( filename, "%s.adc", fileroot );
	fp = fopen( filename, "w" );
	fprintf( fp, "Sample\tTime" );
	for ( chan = 0; chan < nChannels; chan++ ) fprintf( fp, "\tCH%02d", chan );
	fprintf( fp, "\n" );
	for ( smpl = 0; smpl < nAcqSamples; smpl++ ) {
		fprintf( fp, "%d\t%.3f", smpl, acquiredAnalog[smpl].time );
		for ( chan = 0; chan < nChannels; chan++ ) fprintf( fp, "\t%f", acquiredAnalog[smpl].channel[chan] );
		fprintf( fp, "\n" );
	}
	fclose( fp );
	// Note that the file was written.
	monitor->SendEvent( "Data file written: %s", filename );
		
	// Write a file with computed force data.
	ShowStatus( "Writing computed analog data ..." );
	sprintf( filename, "%s.frc", fileroot );
	fp = fopen( filename, "w" );
	fprintf( fp, "Sample\tTime" );
	fprintf( fp, "\tGF" );
	fprintf( fp, "\tLF" );
	fprintf( fp, "\tLFx\tLFy\tLFz" );
	fprintf( fp, "\tACC" );
	fprintf( fp, "\tF1X\tF1Y\tF1Z" );
	fprintf( fp, "\tF2X\tF2Y\tF2Z" );
	fprintf( fp, "\tCOP1X\tCOP1Y\tCOP1Z" );
	fprintf( fp, "\tCOP2X\tCOP2Y\tCOP2Z" );
	fprintf( fp, "\n" );
	for ( smpl = 0; smpl < nAcqSamples; smpl++ ) {
		fprintf( fp, "%d\t%.3f", smpl, acquiredAnalog[smpl].time );
		fprintf( fp, "\t%f", acquiredGripForce[smpl] );
		fprintf( fp, "\t%f", acquiredLoadForceMagnitude[smpl] );
		fprintf( fp, "\t%f\t%f\t%f", acquiredLoadForce[smpl][X], acquiredLoadForce[smpl][Y], acquiredLoadForce[smpl][Z] );
		fprintf( fp, "\t%f", acquiredHighAcceleration[smpl] );
		fprintf( fp, "\t%f\t%f\t%f", acquiredForce[0][smpl][X], acquiredForce[0][smpl][Y], acquiredForce[0][smpl][Z] );
		fprintf( fp, "\t%f\t%f\t%f", acquiredForce[1][smpl][X], acquiredForce[1][smpl][Y], acquiredForce[1][smpl][Z] );
		fprintf( fp, "\t%f\t%f\t%f", acquiredCOP[0][smpl][X], acquiredCOP[0][smpl][Y], acquiredCOP[0][smpl][Z] );
		fprintf( fp, "\t%f\t%f\t%f", acquiredCOP[1][smpl][X], acquiredCOP[1][smpl][Y], acquiredCOP[1][smpl][Z] );
		fprintf( fp, "\n" );
	}
	fclose( fp );
	// Note that the file was written.
	monitor->SendEvent( "Data file written: %s", filename );
	HideStatus();

	
}



