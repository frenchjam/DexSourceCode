/*********************************************************************************/
/*                                                                               */
/*                                   DexApparatus.cpp                            */
/*                                                                               */
/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include "..\DexSimulatorApp\resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <fMessageBox.h>

#include <useful.h>
#include <screen.h>
#include <VectorsMixin.h>


#include <DexTimers.h>
#include "DexMonitorServer.h"
#include "Dexterous.h"
#include "DexTargets.h"
#include "DexTracker.h"
#include "DexSounds.h"
#include "DexApparatus.h"

/***************************************************************************/

const int DexApparatus::negativeBoxMarker = 16;
const int DexApparatus::positiveBoxMarker = 17;
const int DexApparatus::negativeBarMarker = 18;
const int DexApparatus::positiveBarMarker = 19;

DexApparatus::DexApparatus( int n_vertical_targets, 
							int n_horizontal_targets,
							int n_tones, int n_markers ) {
	
	type = DEX_GENERIC_APPARATUS;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nVerticalTargets = n_vertical_targets;
	nHorizontalTargets = n_horizontal_targets;
	nTones = n_tones;
	nMarkers = n_markers;

	// Open a file to store the positions and orientations that were computed from
	// the individual marker positions. This allows us
	// to compare with the position and orientation used by DexMouseTracker
	// to compute the marker positions. 
	char *filename = "DexApparatusDebug.mrk";

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fMessageBox( MB_OK, "DexApparatus", "Error openning file for write:\n %s", filename );
		exit( -1 );
	}
	fprintf( fp, "Time\tPx\tPy\tPz\tQx\tQy\tQz\tQm\n" );

}

DexApparatus::Quit( void ) {

	fclose( fp );

	tracker->Quit();
	monitor->Quit();
	targets->Quit();
	sounds->Quit();
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
	// That's OK for the calculation of the orientation,
	// but now we need to make sure that the targets on the vertical 
	// are where the vertical bar is.

	CodaFrame	frame;
	Vector3	bar_position;
	Vector3 target_position;

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
			items = fscanf( fp, "%f, %f, %f\n", 
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
	float position[3], orientation[4];
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
		tracker->GetCurrentMarkerFrameIntrinsic( frame, unit );
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
		return( fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, "%s\n  Position error: %f\n  Orientation error: %f", msg, distance, angle ) );
	}
	else return( NORMAL_EXIT );
}

/***************************************************************************/

int DexApparatus::PerformTrackerAlignment( const char *msg ) {
	int error_code;
	// Ask the tracker to perform the alignment.
	if ( error_code = tracker->PerformAlignment( negativeBoxMarker, 
													negativeBoxMarker, positiveBoxMarker, 
													negativeBarMarker, positiveBarMarker ) ) {
		// If there is an error, report it to the user.
		// We may need to add information to the message, depending
		// on what the error was.
		return( fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, msg ) );
	}
	else return( NORMAL_EXIT );
}

/***************************************************************************/

// Compute the 3D position vector and 4D orientation quaternion of the manipulandum.
// Takes as input a pointer to a buffer full of one slice of marker data (marker_frame);
// Fills in the position vector pos and the orientation quaternion ori.
bool DexApparatus::ComputeManipulandumPosition( float *pos, float *ori, CodaFrame &marker_frame, Quaternion default_orientation ) {

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
	SubtractVectors( box_and_bar[0], frame.marker[positiveBarMarker].position, frame.marker[negativeBarMarker].position );
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
	if ( int exit_status = tracker->Update() ) exit( exit_status );

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
	float			pos[3], ori[4];
	bool			acquisition_state;
	unsigned long	target_state;
	
	int exit_status;
	
	// Allow each of the components to update as needed.
	if ( exit_status = targets->Update() ) exit( exit_status );
	if ( exit_status = tracker->Update() ) exit( exit_status );
	if ( exit_status = sounds->Update() ) exit( exit_status );
	
	// Compute the current state of the manipulandum;
	// Note that I no longer cache these information.
	// It is more efficient for the other routines to call them as
	// necessary.
	manipulandum_visible = GetManipulandumPosition( pos, ori );
			
	// Send state information to the ground.
	acquisition_state = tracker->GetAcquisitionState();
	target_state = targets->GetTargetState();
	monitor->SendState( acquisition_state, target_state, manipulandum_visible, pos, ori );
	
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

int DexApparatus::SignalError( unsigned int mb_type, const char *message ) {
		
	DexTimer move_timer;

	for ( int blinks = 0; blinks < N_ERROR_BLINKS; blinks++ ) {
		
		targets->SetTargetState( 0x55555555 );
		DexTimerSet( move_timer, BLINK_PERIOD );
		while( !DexTimerTimeout( move_timer ) ) Update();
		targets->SetTargetState( ~0x55555555 );
		DexTimerSet( move_timer, BLINK_PERIOD );
		while( !DexTimerTimeout( move_timer ) ) Update();
		
	}
	
	targets->SetTargetState( ~0x00000000 );
	DexTimerSet( move_timer, BLINK_PERIOD );
	while( !DexTimerTimeout( move_timer ) ) Update();
	
	monitor->SendEvent( message );
	return( MessageBox( NULL, message, "DEX Message", mb_type ) );
	
}

// fprintf version of the above.

int DexApparatus::fSignalError( unsigned int mb_type, const char *format, ... ) {

	va_list args;
	char message[10240];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);

	return( SignalError( mb_type, message ) );

}

// This method is implemented using the primitive commands of DexApparatus,
// not by communicating with the hardware directly.
// It is not a command that will generate a single script command.
// Rather, by calling this command, the primitives required to generate the 
// behavior will be written to the script.
// Thus, the Dex interpreter does not have to handle this command directly.

int DexApparatus::SignalNormalCompletion( const char *message ) {
		
	int status;

	for ( int blinks = 0; blinks < N_NORMAL_BLINKS; blinks++ ) {
		
		TargetsOff();
		Wait( BLINK_PERIOD );
		
		SetTargetState( ~0 );
		Wait( BLINK_PERIOD );
		
	}
	
	status = WaitSubjectReady( message );
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

int DexApparatus::WaitSubjectReady( const char *message ) {
	
	Update();
	monitor->SendEvent( "WaitSubjectReady - %s", message );
	// There is a problem here. Update should be called while waiting for the subject's
	// response. Update() could perhaps be run in a thread.
	// But in the real system, the subsystems will presumably run in background 
	// threads, so this is not an issue.
	int response = MessageBox( NULL, message, "DEX", MB_OKCANCEL | MB_ICONQUESTION );
	Update();
	if ( response == IDCANCEL ) {
		monitor->SendEvent( "Manual Abort from WaitSubjectReady." );
		return( ABORT_EXIT );
	}
	
	return( NORMAL_EXIT );
	
}

// fprintf version of the above.

int DexApparatus::fWaitSubjectReady( const char* format, ... ) {
	
	va_list args;
	char message[10240];
	
	va_start(args, format);
	vsprintf(message, format, args);
	va_end(args);

	return( WaitSubjectReady( message ) );

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
	
	// Current simulated configuration.
	int current_posture;
	int current_bar_position;
	int current_tapping;
	
	// Record that we started this step.
	monitor->SendEvent( "SelectAndCheckConfiguration - Desired: [%d %d %d].", posture, bar_position, tapping );
	
	while ( 1 ) {
		
		// Signal to the ground what is the current configuration.
		SignalConfiguration();
		
		// Check what is the current configuration to be compared with the desired.
		// Super classes implement tests to detect the target frame orientation and
		// the position of the vertical target bar.
		current_posture = Posture();
		current_bar_position = BarPosition();

		// The real DEX hardware cannot detect the deployment of the tapping surfaces.
		// I keep it here just for show. 
		current_tapping = TappingDeployment();
		
		if (  ( posture == DONT_CARE || posture == current_posture ) &&
			  ( bar_position == DONT_CARE || bar_position == current_bar_position ) && 
			  ( tapping == DONT_CARE || tapping == current_tapping ) 
		   ) {
			monitor->SendEvent( "Successful configuration check." );
			return( NORMAL_EXIT );
		}
		
		// Signal to the subject that the configuration is currently not correct.
		// Allow them to retry to achieve the desired configuration, to ignore and move on, or to abort the session.
		int response = fSignalError(  MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION,
			"Configuration incorrect.\n\nDesired configuration:\n\n  Subject Restraint:     %s\n  Target Bar:               %s\n  Tapping Surfaces:     %s", 
			PostureString[posture], TargetBarString[bar_position], TappingSurfaceString[tapping] );
		if ( response == IDABORT ) {
			monitor->SendEvent( "Manual Abort from SelectAndCheckConfiguration." );
			return( ABORT_EXIT );
		}
		if ( response == IDIGNORE ) {
			monitor->SendEvent( "Ignore Error from SelectAndCheckConfiguration." );
			return( IGNORE_EXIT );
		}
		// If we get here, it's a retry. Loop again to check if the change was successful.
		
	}
	
	// Should never get here.
	
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
	
	DexTimer move_timer;
	DexTimerSet( move_timer, duration );
	while( !DexTimerTimeout( move_timer ) ) Update(); // This does the updating.
	
}

/***************************************************************************/

//
// Wait until the manipulandum is placed at a target position.
//
int DexApparatus::WaitUntilAtTarget( int target_id, const float desired_orientation[4],
									float position_tolerance[3], float orientation_tolerance,
									float hold_time, float timeout, char *msg  ) {
	
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
	monitor->SendEvent( "WaitUntilAtTarget - Start." );
	DexTimerSet( timeout_timer, timeout );
	while ( 1 ) {
		
		led_state = 0;
		TargetsOff();
		TargetOn( target_id );
		DexTimerSet( blink_timer, waitBlinkPeriod );
		do {

			manipulandum_visible = GetManipulandumPosition( manipulandum_position, manipulandum_orientation );
			
			// Compare the computed orientation to the desired orientation.
			misorientation = ToDegrees( AngleBetween( desired_orientation, manipulandum_orientation ) ) ;
			if ( DexTimerTimeout( timeout_timer ) ) {
				
				// Timeout has been reached. Signal the error to the user.
				if ( !msg ) msg = "Time to reach target exceeded.\n Is the manipulandum visible?\n Is the manipulandum upright?\n";
				mb_reply = fSignalError( MB_ABORTRETRYIGNORE | MB_ICONEXCLAMATION, 
					"%s\n  Target ID: %d\n  Max time: %.2f\n  Manipulandum Visible: %s\n  Desired orientation: <%.3f %.3f %.3f %.3f>\n  Orientation Error: %.0f degrees",
					msg, target_id,
					timeout, ( manipulandum_visible ? "yes" : "no" ), 
					desired_orientation[X], desired_orientation[Y],
					desired_orientation[Z], desired_orientation[M],
					misorientation );
				// Exit, signalling that the subject wants to abort.
				if ( mb_reply == IDABORT ) {
					monitor->SendEvent( "Manual Abort from WaitUntilAtTarget." );
					return( ABORT_EXIT );
				}
				// Ignore the error and move on.
				if ( mb_reply == IDIGNORE ) {
					monitor->SendEvent( "Ignore Timeout from WaitUntilAtTarget." );
					return( IGNORE_EXIT );
				}
				
				// Try again for another timeout period. Thus, retry restarts the step, 
				// not the entire script.
				monitor->SendEvent( "Retry after Timeout from WaitUntilAtTarget." );
				DexTimerSet( timeout_timer, waitTimeLimit );
				
			}
			
			// This toggles the target LED on and off to attract attention.
			if ( DexTimerTimeout( blink_timer ) ) {
				TargetsOff();
				led_state = !led_state;
				if ( led_state ) TargetOn( target_id );
				DexTimerSet( blink_timer, waitBlinkPeriod );
			}
			
			// The apparatus class has an Update() method that acquires the most recent 
			// marker positions and computes the position and orientation of the maniplandum.
			Update();

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
				monitor->SendEvent( "WaitUntilAtTarget - Success." );
				return( NORMAL_EXIT );
			}
			// Continue to perform required updates during the wait period.
			Update();
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

int DexApparatus::WaitUntilAtVerticalTarget( int target_id, const float desired_orientation[4], float position_tolerance[3], float orientation_tolerance, float hold_time, float timeout, char *msg ) {
	return( WaitUntilAtTarget( target_id, desired_orientation, position_tolerance, orientation_tolerance, hold_time, timeout, msg ) );
}

int DexApparatus::WaitUntilAtHorizontalTarget( int target_id, const float desired_orientation[4], float position_tolerance[3], float orientation_tolerance, float hold_time, float timeout, char *msg ) {
	return( WaitUntilAtTarget( nVerticalTargets + target_id, desired_orientation, position_tolerance, orientation_tolerance, hold_time, timeout, msg  ) );
}

/*********************************************************************************/
/*                                                                               */
/*                                Stimulus Control                               */
/*                                                                               */
/*********************************************************************************/

// Set the state of the target LEDs.

void DexApparatus::SetTargetState( unsigned long target_state ) {
	targets->SetTargetState( target_state );
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

void DexApparatus::VerticalTargetOn( int id ) {
	TargetOn( id );
}

void DexApparatus::HorizontalTargetOn( int id ) {
	TargetOn( nVerticalTargets + id );
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
void DexApparatus::SetSoundState( int tone, int volume ) {
	sounds->SetSoundState( tone, volume );
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
// TO DO: Should be updated to the new specification where SaveAcqusition 
// does not exist.
void DexApparatus::StartAcquisition( float max_duration ) {
	// Reset the list of target and sound events.
	nEvents = 0;
	// Tell the tracker to start acquiring.
	tracker->StartAcquisition( max_duration );
	// Keep track of how long we have been acquiring.
	DexTimerSet( trialTimer, max_duration );
	// Tell the ground what we just did.
	monitor->SendEvent( "Acquisition started. Max duration: %f seconds.", max_duration );
	// Note the time of the acqisition start.
	MarkEvent( ACQUISITION_START );
}
void DexApparatus::StopAcquisition( void ) {
	MarkEvent( ACQUISITION_STOP );
	tracker->StopAcquisition();
	monitor->SendEvent( "Acquisition terminated." );
	// Retrieve the marker data.
	nAcqFrames = tracker->RetrieveMarkerFrames( acquiredPosition, DEX_MAX_MARKER_FRAMES );
	// Compute the manipulandum positions.
	for ( int i = 0; i < nAcqFrames; i++ ) {
		// Here I should compute the manipulandum position and orientation.
		acquiredManipulandumState[i].visibility = acquiredPosition[i].marker[0].visibility;
		acquiredManipulandumState[i].time = acquiredPosition[i].time;
		CopyVector( acquiredManipulandumState[i].position, acquiredPosition[i].marker[0].position );
		CopyVector( acquiredManipulandumState[i].orientation, nullQuaternion );

	}
	// Send the recording by telemetry to the ground for monitoring.
	monitor->SendRecording( acquiredManipulandumState, nAcqFrames, INVISIBLE );
}
void DexApparatus::SaveAcquisition( const char *tag ) {
	
	FILE *fp;
	
	// TO DO: Automatically generate a file name.
	// Here we use the same name each time, but just add the tag.
	char filename[1024];
	sprintf( filename, "DexSimulatorOutput.%s.csv", tag );
	
	// Write the data file.
	fp = fopen( filename, "w" );
	fprintf( fp, "Sample,Time,X,Y,Z\n" );
	for ( int i = 0; i < nAcqFrames; i++ ) {
		fprintf( fp, "%d,%.3f,%.3f,%.3f,%.3f\n", i / 3, 
			acquiredManipulandumState[i].time, 
			acquiredManipulandumState[i].position[X], 
			acquiredManipulandumState[i].position[Y], 
			acquiredManipulandumState[i].position[Z] );
	}
	fclose( fp );
	
	// Note that the file was written.
	monitor->SendEvent( "Data file written: %s", filename );
	
}

/***************************************************************************/
/*                                                                         */
/*                             DexCodaApparatus                            */
/*                                                                         */
/***************************************************************************/

// A combination of simulated targets on the computer screen the real CODA tracker.

DexCodaApparatus::DexCodaApparatus(  int n_vertical_targets, 
									 int n_horizontal_targets,
									 int n_tones, int n_markers ) {
	
	type = DEX_CODA_APPARATUS;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nTones = n_tones;
	nMarkers = n_markers;
	nCodas = 2;
	nEvents = 0;
	
	// Create a window to monitor the experiment.
	monitor = new DexMonitorServer( n_vertical_targets, n_horizontal_targets, nCodas );
	
	// Initialize target system.
	targets = new DexScreenTargets();
	
	// Initialize target system.
	sounds = new DexScreenSounds();

	// Use the real Coda tracker.
	tracker = new DexCodaTracker();
	
	// Load the most recently defined target positions.
	LoadTargetPositions();

	// Initialize the tracker hardware.
	tracker->Initialize();

	// Start with all the targets off.
	TargetsOff();
	
}

/***************************************************************************/
/*                                                                         */
/*                             DexRTnetApparatus                           */
/*                                                                         */
/***************************************************************************/

// A combination of simulated targets on the computer screen the real CODA tracker.

DexRTnetApparatus::DexRTnetApparatus(  int n_vertical_targets, 
									   int n_horizontal_targets,
									   int n_tones, 
                                       int n_markers ) {
	
	type = DEX_RTNET_APPARATUS;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nTones = n_tones;
	nMarkers = n_markers;
	nEvents = 0;

	DexRTnetTracker *tracker_local;
		
	// Initialize target system.
	targets = new DexScreenTargets();
	
	// Initialize target system.
	sounds = new DexScreenSounds();

	// Use the real Coda tracker.
	tracker_local = new DexRTnetTracker();
	tracker = tracker_local;

	// Ask the tracker how many Codas are being used.
	nCodas = tracker_local->nCodas;
	
	// Create a window to monitor the experiment.
	monitor = new DexMonitorServer( n_vertical_targets, n_horizontal_targets, nCodas );

	// Load the most recently defined target positions.
	LoadTargetPositions();
  
	// Initialize the tracker hardware.
	tracker->Initialize();
  
	// Start with all the targets off.
	TargetsOff();
  	
}

/***************************************************************************/
/*                                                                         */
/*                             DexMouseApparatus                           */
/*                                                                         */
/***************************************************************************/

//
// A laptop version of the DEX apparatus.
// Uses the mouse to simulate tracking of the manipulandum.
// Uses a dialog box to simulate setting up the apparatus in different configurations.
//

// Mesage handler for dialog box.

BOOL CALLBACK dexApparatusDlgCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
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

/***************************************************************************/

DexMouseApparatus::DexMouseApparatus( HINSTANCE hInstance, 
									 int n_vertical_targets, 
									 int n_horizontal_targets,
									 int n_tones, int n_markers ) {
	
	type = DEX_MOUSE_APPARATUS;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nTones = n_tones;
	nMarkers = n_markers;
	nCodas = 2;
	nEvents = 0;
	
	// Initialize dialog box.
	dlg = CreateDialog(hInstance, (LPCSTR)IDD_CONFIG, HWND_DESKTOP, dexApparatusDlgCallback );
	CheckRadioButton( dlg, IDC_SEATED, IDC_SUPINE, IDC_SEATED ); 
	CheckRadioButton( dlg, IDC_LEFT, IDC_RIGHT, IDC_LEFT ); 
	CheckRadioButton( dlg, IDC_HORIZ, IDC_VERT, IDC_VERT );
	CheckRadioButton( dlg, IDC_FOLDED, IDC_EXTENDED, IDC_FOLDED );
	CheckDlgButton( dlg, IDC_CODA_ALIGNED, true );
	CheckDlgButton( dlg, IDC_CODA_POSITIONED, true );
	ShowWindow( dlg, SW_SHOW );
	
	// Create a link to monitor the experiment.
	monitor = new DexMonitorServer( n_vertical_targets, n_horizontal_targets, nCodas );
	
	// Initialize target system.
	targets = new DexScreenTargets( n_vertical_targets, n_horizontal_targets );

	// Initialize sounds.
	sounds = new DexScreenSounds( n_tones );
	
	// Use a virtual mouse tracker in place of the coda.
	tracker = new DexMouseTracker( dlg );
	
	// Load the most recently defined target positions.
	LoadTargetPositions();
  
	// Initialize the tracker hardware.
	tracker->Initialize();
  
	// Start with all the targets off.
	TargetsOff();
  
}

/***************************************************************************/

#if 0 // Ingore virtual apparatus for now.

/***************************************************************************/
/*                                                                         */
/*                          DexVirtualApparatus                            */
/*                                                                         */
/***************************************************************************/

// A combination of simulated targets on the computer screen and a simulated
// tracker that magically moves the manipulandum around.

DexVirtualApparatus::DexVirtualApparatus( int n_vertical_targets, 
									 int n_horizontal_targets,
									 int n_tones, int n_codas ) {
	
	type = DEX_VIRTUAL_APPARATUS;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nCodas = n_codas;
	nTones = n_tones;
	
	// Create a server to transmit monitored data to the client.
	monitor = new DexMonitorServer( n_vertical_targets, n_horizontal_targets, n_codas );
	
	// Initialize target system.
	targets = new DexScreenTargets( 5 );
	
	// Use a virtual tracker in place of the coda.
	tracker = new DexVirtualTracker( targets );
	tracker->Initialize();
	manipulandumVisible = true;
	
	
	StopAcquisition();
	TargetsOff();
	
}

#endif 