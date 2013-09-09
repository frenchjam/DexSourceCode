/*********************************************************************************/
/*                                                                               */
/*                                   DexCompiler.cpp                             */
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

#include <OpenGLViewpoints.h>

#include <VectorsMixin.h> 

#include "Dexterous.h"
#include "DexMonitorServer.h"
#include "DexSounds.h"
#include "DexTargets.h"
#include "DexTracker.h"
#include "DexApparatus.h"


/***************************************************************************/
/*                                                                         */
/*                                DexCompiler                              */
/*                                                                         */
/***************************************************************************/

//
// Doesn't actually do anything. Instead, it just writes out each essential
// command to a script to be executed later by the interpreter.
//

//  !!!!!!!!!!!!!!!!!!!! THIS IS NOT UP TO DATE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   It may not cover all the required commands.
	
DexCompiler::DexCompiler( char *filename ) {

	script_filename = filename;

}

void DexCompiler::Initialize( void ) {
	
	
	nVerticalTargets = N_VERTICAL_TARGETS;
	nHorizontalTargets = N_HORIZONTAL_TARGETS;
	nTargets = nVerticalTargets + nHorizontalTargets;

	// Create bit masks for the horizontal and vertical targets.
	// If you change something here, don't forget to change it in the base class DexApparatus as well.
	verticalTargetMask = 0;
	for ( int i = 0; i < nVerticalTargets; i++ ) verticalTargetMask = ( verticalTargetMask << 1 ) + 1;
	horizontalTargetMask = 0;
	for ( i = 0; i < nHorizontalTargets; i++ ) horizontalTargetMask = ( horizontalTargetMask << 1 ) + 1;
	horizontalTargetMask = horizontalTargetMask << nVerticalTargets;

	nTones = N_TONES;
	nCodas = N_CODAS;
	nMarkers = N_MARKERS;
	nChannels = N_CHANNELS;


	// Open a file into which we will write the script.
	fp = fopen( script_filename, "w" );
	if ( !fp ) {
		char message[1024];
		sprintf( message, "Error opening script file for write:\n  %s", script_filename );
	}

	// Write out comments as a header to the file.

	char date_str [9];
	char time_str [9];
	_strdate( date_str);
	_strtime( time_str );

	fprintf( fp, "#\n" );
	fprintf( fp, "# Filename: %s\n", script_filename );
	fprintf( fp, "# Created:  %s %s\n", date_str, time_str );
	fprintf( fp, "#\n" );
	fprintf( fp, "\n" );
	fprintf( fp, "# This script is autmatically generated by the PsyPhy DEX simulator. \n" );
	fprintf( fp, "# You are strongly discouraged from editing this file directly. \n" );
	fprintf( fp, "\n" );

	// Keep track of step number.
	// Step numbers are output as comments just before each executable step in the script file.
	// This allows one to easily find offending commands when the DEX interpreter signals an error.

	nextStep = 0;

	
	// The base class sets the targets and sounds off at initialization.
	// DEX does that automatically, so I removed those commands from the compiler.
	// Comment( "Start with all the targets and sounds off, but don't signal the event." );
	// SetTargetStateInternal( 0x00000000 );
	// SetSoundStateInternal( 0, 0 );

	
}

void DexCompiler::CloseScript( void ) {
	fclose( fp );
}

void DexCompiler::Quit( void ) {

	CloseScript();

}

/***************************************************************************/

// Translate the full-width target bit pattern used by the simulator into
// separate bit patterns for the horizontal and vertical targets used by
// the DEX hardware.

unsigned short DexCompiler::verticalTargetBits( unsigned long targetBits ) {
	return( targetBits & verticalTargetMask );
}
unsigned short DexCompiler::horizontalTargetBits( unsigned long targetBits ) {
	return( (targetBits & horizontalTargetMask) >> nVerticalTargets );
}

unsigned short DexCompiler::verticalTargetBit( int target_id ) {
	if ( target_id < nVerticalTargets ) return( 0x01 << target_id );
	else return( 0 );
}
unsigned short DexCompiler::horizontalTargetBit( int target_id) {
	if ( target_id >= nVerticalTargets ) return( 0x01 << ( target_id - nVerticalTargets ) );
	else return( 0 );
}

/***************************************************************************/

// Message strings have to be quoted by the DEX interpreter.
// This puts quotes into the string itself.
// Also, they have a limited length and for the moment commas have to be replaced.

// The return value is a pointer to a static string that gets overwritten every time
//  this routine is called. Since in general the result is used just once in a printf
//  statement, this should not be a problem. But if you want to save the result, 
//  you need to copy the resulting string.

char *DexCompiler::quoteMessage( const char *message ) {

	static char result[ 2 * DEX_MAX_MESSAGE_LENGTH ];

	if ( !message ) {
		strcpy( result, "\"\"" );
		return( result );
	}

	int j = 0;
	result[j++] = '\"';

	// Limit the message to the size handled by DEX and replace some special characters.
	for ( int i = 0; i < DEX_MAX_MESSAGE_LENGTH && j < sizeof( result ) - 1 && message[i] != 0; i++ ) {
		// Transform linebreaks into \n.
		if ( message[i] == '\n' ) {
			if ( i == DEX_MAX_MESSAGE_LENGTH - 1) break;
			result[j++] = '\\';
			result[j++] = 'n';
		}
		// The DEX parser is messing up on commas, even if they are in quotes.
		else if ( message[i] == ',' ) {
			result[j++] = ';';
		}
		// Otherwise, just copy.
		else result[j++] = message[i];

	}
	result[j++] = '\"';
	result[j++] = 0;

	return( result );

}


/***************************************************************************/

int DexCompiler::WaitSubjectReady( const char *message ) {

	// There is no provision yet for an image file, so just put an empty field.
	// I did not foresee a timeout for this command. I am setting the timeout 
	// to the maximum.
	AddStepNumber();
	fprintf( fp, "CMD_WAIT_SUBJ_READY, %s, %s, %.0f\n", quoteMessage( message ), "", DEX_MAX_TIMEOUT );
	return( NORMAL_EXIT );
}

/***************************************************************************/

void DexCompiler::Wait( double duration ) {
	AddStepNumber();
	fprintf( fp, "CMD_WAIT, %.0f\n", duration * 10.0 );
}

/***************************************************************************/

int DexCompiler::WaitUntilAtTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {

	AddStepNumber();
	fprintf( fp, "CMD_WAIT_MANIP_ATTARGET, 0x%04x, 0x%04x, %f, %f, %f, %f, %.0f, %.0f, %.0f, %.0f, %.0f, %.0f, %s\n",
		horizontalTargetBit( target_id ), verticalTargetBit( target_id ), 
		desired_orientation[X], desired_orientation[Y], desired_orientation[Z], desired_orientation[M], 
		position_tolerance[X], position_tolerance[Y], position_tolerance[Z], orientation_tolerance,
		hold_time, timeout, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

/***************************************************************************/

int	 DexCompiler::WaitCenteredGrip( float tolerance, float min_force, float timeout, char *msg ) {
	static bool once = false;
	if ( timeout > DEX_MAX_TIMEOUT && !once ) {
		once = true;
		MessageBox( NULL, "Warning - Timeout exceeds limit.", "DexCompiler", MB_OK );
	}
	if ( timeout < 0 && !once ) {
		once = true;
		MessageBox( NULL, "Warning - Timeout cannot be negative.", "DexCompiler", MB_OK );
	}
	AddStepNumber();
	fprintf( fp, "CMD_WAIT_MANIP_GRIP, %f, %.0f, %f, \"%s\"\n", min_force, tolerance, timeout, msg );
	return( NORMAL_EXIT );
}

int	DexCompiler::WaitDesiredForces( float min_grip, float max_grip, 
								 float min_load, float max_load,
								 Vector3 direction, float filter_constant,
								 float hold_time, float timeout, const char *msg ) {

	AddStepNumber();
	fprintf( fp, "CMD_WAIT_MANIP_GRIPFORCE, %f, %f, %f, %f, %f, %f, %f, %.0f, %.0f, %s\n",
		min_grip, max_grip, min_load, max_load, direction[X], direction[Y], direction[Z], 
		hold_time * 10.0, timeout, quoteMessage( msg ) );

	return( NORMAL_EXIT );
}



int DexCompiler::WaitSlip( float min_grip, float max_grip, 
								 float min_load, float max_load, 
								 Vector3 direction,
								 float filter_constant,
								 float slip_threshold, 
								 float timeout, const char *msg ) {

	AddStepNumber();
	fprintf( fp, "CMD_WAIT_MANIP_SLIP, %f, %f, %f, %f, %f, %f, %f, %.0f, %.0f, %s\n",
		min_grip, max_grip, min_load, max_load, direction[X], direction[Y], direction[Z], 
		slip_threshold, timeout, quoteMessage( msg ) );

	return( NORMAL_EXIT );
}

/***************************************************************************/

int DexCompiler::SelectAndCheckConfiguration( int posture, int bar_position, int tapping ) {
	char message[256];
	sprintf( message, "Please set the following configuration:\\n   Posture: %s   Target Bar: %s",
		( posture ? "supine" : "seated" ),
		( bar_position ? "Right" : "Left" ));
	AddStepNumber();
	// I am putting the maximum for the timeout, because I don't believe that this one should timeout.
	// I had three states for posture and bar position, including an 'indifferent' state. 
	// Here I translate those to the 0 and 1 defined by DEX.
	fprintf( fp, "CMD_CHK_HW_CONFIG, %s, %s, %d, %d, %.0f \n", quoteMessage( message ), "", 
		( posture == PostureSeated ? 0 : 1 ), ( bar_position == TargetBarLeft ? 1 : 0 ), DEX_MAX_TIMEOUT );
	return( NORMAL_EXIT );
}

/*********************************************************************************/

int DexCompiler::SelectAndCheckMass( int mass ) {

	int mass_id;

	if ( mass == MassIndifferent ) return( NORMAL_EXIT );
	if ( mass == MassNone ) MessageBox( NULL, "DEX does not handle MassNone.", "DexApparatus", MB_OK );
	else if ( mass == MassSmall ) mass_id = 0;
	else if ( mass == MassLarge ) mass_id = 2;
	else mass_id = 1;

	AddStepNumber();
	fprintf( fp, "Select mass: , , %d, %.0f\n", mass_id, DEX_MAX_TIMEOUT );
	return( NORMAL_EXIT );
}



/**************************************************************************************************************/

// These commands are used to set the target and sound states.
// Here I have replaced the lowest level command which does that and only that.
// The higher level commands SetTargetStateInternal() and SetSoundState() call
//  these routines, and they automatically make calls to log the events.
// If CMD_CTRL_TARGETS and CMD_CTRL_TONE also automatically log events, then
//  one should change these funtions to overlay SetTargetStateInternal() and SetSoundState() instead.

void DexCompiler::SetTargetStateInternal( unsigned long target_state ) {

	// I assumed that targets will be set with a single 32-bit word.
	// Instead, DEX treats horizontal and vertical with separate 16-bit words.
	// Here I reconstuct the two DEX bits patterns from the simulator bit pattern.
	unsigned short vstate, hstate;
	vstate = verticalTargetBits( target_state );
	hstate = horizontalTargetBits( target_state );
	AddStepNumber();
	fprintf( fp, "CMD_CTRL_TARGETS, 0x%04x, 0x%04x\n", hstate, vstate );
}

void DexCompiler::SetSoundStateInternal( int tone, int volume ) {
	static bool once = false;
	if ( volume != 0 && volume != 1 && !once ) {
		MessageBox( NULL, "Warning - Sound volume on DEX is 0 or 1", "DexCompiler", MB_OK );
		once = true;
	}
	if ( ( tone < 0 || tone >= nTones ) && !once ) {
		MessageBox( NULL, "Warning - Tone is out of range.", "DexCompiler", MB_OK );
		once = true;
	}
	AddStepNumber();
	// On DEX, the bit is actually 'mute' or 'not mute'. This volume (on or off) to mute (on or off).
	fprintf( fp, "CMD_CTRL_TONE, %d, %d\n", ( volume ? 0 : 1 ), tone );
}

/**************************************************************************************************************/

int DexCompiler::CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_MANIP_VISIBILITY, %f, %f, %s\n", 
		max_cumulative_dropout_time, max_continuous_dropout_time, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

// DEX does not implement overrun checks.
int DexCompiler::CheckOverrun(  const char *msg ) {
	return( NORMAL_EXIT );
}

int DexCompiler::CheckMovementAmplitude(  double min, double max, 
										   double dirX, double dirY, double dirZ,
										   const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_MOVEMENTS_AMPL, %.0f, %.0f, %f, %f, %f, %s\n", 
		min, max, dirX, dirY, dirZ, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckMovementCycles(  int min_cycles, int max_cycles, 
							float dirX, float dirY, float dirZ,
							float hysteresis, const char *msg ) {
	AddStepNumber();
	return( NORMAL_EXIT );
}

int DexCompiler::CheckEarlyStarts(  int n_false_starts, float hold_time, float threshold, float filter_constant, const char *msg ) {
	AddStepNumber();
	MessageBox( NULL, "CheckEarlyStarts()\nWhat about the filter constant?!?!", "DexCompiler", MB_OK | MB_ICONQUESTION );
	fprintf( fp, "CMD_CHK_EARLYSTARTS, %d, %.0f, %f, %s\n", n_false_starts, hold_time * 10, threshold, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}
int DexCompiler::CheckCorrectStartPosition( int target_id, float tolX, float tolY, float tolZ, int max_n_bad, const char *msg ) {

	unsigned short horizontal = horizontalTargetBit( target_id );
	unsigned short vertical = verticalTargetBit( target_id );

	AddStepNumber();
	fprintf( fp, "CMD_CHK_START_POS, 0x%04x, 0x%04x, %.0f, %.0f, %.0f, %d, %s\n", 
		horizontal, vertical, tolX, tolY, tolZ, max_n_bad, quoteMessage( msg ) );
	
	return( NORMAL_EXIT );
}
int DexCompiler::CheckMovementDirection(  int n_false_directions, float dirX, float dirY, float dirZ, float threshold, const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_MOVEMENTS_DIR, %d, %f, %f, %f, %.1f, %s\n", 
		n_false_directions, dirX, dirY, dirZ, threshold, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}
int DexCompiler::CheckForcePeaks( float min_force, float max_force, int max_bad_peaks, const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_COLLISIONFORCE, %f, %f, %d, %s\n", 
		min_force, max_force, max_bad_peaks, quoteMessage( msg ) );
	return( NORMAL_EXIT );
};

void DexCompiler::ComputeAndNullifyStrainGaugeOffsets( void ) {
	AddStepNumber();
	fprintf( fp, "CMD_NULLIFY_FORCES, ERROR????\n" );
};


/**************************************************************************************************************/

void DexCompiler::StartAcquisition( const char *tag, float max_duration ) {
	// DEX ignores the max duration parameter.
//	fprintf( fp, "CMD_ACQ_START, \"%s\"\n", tag ); 
	AddStepNumber();
	fprintf( fp, "CMD_ACQ_START, %s\n", tag );  // Need to confirm if the tag should be quoted or not.
	// Note the time of the acqisition start.
	MarkEvent( ACQUISITION_START );
}

int DexCompiler::StopAcquisition( const char *msg ) {
	MarkEvent( ACQUISITION_STOP );
	AddStepNumber();
	fprintf( fp, "CMD_ACQ_STOP, %s\n", quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

/*********************************************************************************/

int DexCompiler::PerformTrackerAlignment( const char *message ) {
	AddStepNumber();
	fprintf( fp, "CMD_ALIGN_CODA, %s\n", quoteMessage( message ) );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_CODA_ALIGNMENT, 0x%08lx, %.0f, %d, %s\n", marker_mask, tolerance, n_good, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckTrackerFieldOfView( int unit, unsigned long marker_mask, 
										   float min_x, float max_x,
										   float min_y, float max_y,
										   float min_z, float max_z, const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_CODA_FIELDOFVIEW, %d, 0x%08lx, %.0f, %.0f, %.0f, %.0f, %.0f, %.0f, %s\n", 
		unit + 1, marker_mask, min_x, max_x, min_y, max_y, min_z, max_z, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckTrackerPlacement( int unit, 
										const Vector3 expected_pos, float p_tolerance,
										const Quaternion expected_ori, float o_tolerance,
										const char *msg ) {
	AddStepNumber();
	fprintf( fp, "CMD_CHK_CODA_PLACEMENT, %d, %.0f, %.0f, %.0f, %f, %f, %f, %f, %.0f, %.0f, %s\n", 
		unit + 1, expected_pos[X], expected_pos[Y], expected_pos[Z],  
		expected_ori[X], expected_ori[Y], expected_ori[Z], expected_ori[M], 
		p_tolerance, o_tolerance, quoteMessage( msg ) );
	return( NORMAL_EXIT );
}

/*********************************************************************************/


void DexCompiler::MarkEvent( int event, unsigned long param ) {
	AddStepNumber();
	fprintf( fp, "CMD_LOG_EVENT, %d\n", event );
}

void DexCompiler::SignalEvent( const char *event ) {
	AddStepNumber();
	// Log a message, without showing it to the subject.
	fprintf( fp, "CMD_LOG_MESSAGE, 0, %s\n", quoteMessage( event ) );
}

void DexCompiler::ShowStatus (const char *message ) {
	AddStepNumber();
	// Log the message and show it on the DEX screen.
	fprintf( fp, "CMD_LOG_MESSAGE, 1, %s\n", quoteMessage( message ) );
}

void DexCompiler::Comment( const char *txt ) {
	fprintf( fp, "\n# %s\n", txt );
}

void DexCompiler::AddStepNumber( void ) {
	fprintf( fp, "# Step %08d\n", nextStep++ );
}

/*********************************************************************************/

// The following routines do not output anything to the scripts. 
// They represent stuff that cannot be executed by DEX.
// Here we flag them during compilation, in case the user tries to
// include them in a script.

void DexCompiler::UnhandledCommand( const char *cmd ) {
	char msg[2048];
	sprintf( msg, "Warning - Command is not handled by DEX script processor: %s", cmd );
	MessageBox( NULL, msg, "DEX Compiler Warning", MB_OK | MB_ICONHAND );
}
void DexCompiler::SetTargetPositions( void ) { 
	static bool first = true;
	if ( first ) UnhandledCommand( "SetTargetPositions()" );
	first = false;
}
void DexCompiler::SignalConfiguration( void ) {
	static bool first = true;
	if ( first ) UnhandledCommand( "SignalConfiguration()" );
	first = false;
}

int DexCompiler::CheckAccelerationPeaks( float min_amplitude, float max_amplitude, int max_bad_peaks, const char *msg ){ 
	static bool first = true;
	if ( first ) UnhandledCommand( "CheckAccelerationPeaks()" );
	first = false;
	return( NORMAL_EXIT ); 
}

/*********************************************************************************/

/* 
* The DexCompiler apparatus generates a script of the top-level commands.
* Here we run the set of steps defined by such a script, instead of running 
* a protocol defined by a C subroutine (as above).
*/

//  !!!!!!!!!!!!!!!!!!!! THIS IS NOT UP TO DATE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//   It may not cover all the required commands.

// This could instead be a method in DexApparatus.

int RunScript( DexApparatus *apparatus, const char *filename ) {
	
	FILE *fp;
	int status = 0;
	char line[1024];
	
	fp = fopen( filename, "r" );
	if ( !fp ) {
		char message[1024];
		sprintf( message, "Error opening script file:\n  %s", filename );
		MessageBox( NULL, message, "DEX Error", MB_OK );
		return( ERROR_EXIT );
	}
	
	while ( fgets( line, sizeof( line ), fp ) ) {
		
		char token[1024];
		
		sscanf( line, "%s", token );
		
		if ( !strcmp( token, "SelectAndCheckConfiguration" ) ) {
			
			int posture, target_config, tapping_config;
			sscanf( line, "%s %d %d %d", token, &posture, &target_config, &tapping_config );
			
			// Select which configuration of the hardware should be used.
			// Verify that it is in the correct configuration, and if not, 
			//  give instructions to the subject about what to do.
			status = apparatus->SelectAndCheckConfiguration( posture, target_config, tapping_config );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
		
		if ( !strcmp( token, "WaitSubjectReady" ) ) {
			
			char *prompt = strpbrk( line, " \t" );
			if ( !prompt ) prompt = "Ready?";
			else prompt++;
			
			// Instruct subject to take the appropriate position in the apparatus
			//  and wait for confimation that he or she is ready.
			status = apparatus->WaitSubjectReady( prompt );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
		
		if ( !strcmp( token, "WaitUntilAtTarget" ) ) {
			
			int target = 0;
			sscanf( line, "%s %d", token, &target );
			
			// Wait until the subject gets to the target before moving on.
			status = apparatus->WaitUntilAtVerticalTarget( target );
			if ( status == ABORT_EXIT ) exit( status );
		}
		
		if ( !strcmp( token, "StartAcquisition" ) ) {
			float max_duration = 120.0;
			char tag[256];

			sscanf( line, "%s %s %f", token, tag, &max_duration );
			// Start acquiring data.
			apparatus->StartAcquisition( tag, max_duration );
		}
		
		if ( !strcmp( token, "Wait" ) ) {
			
			double duration = 0.0;
			sscanf( line, "%s %lf", token, &duration );
			
			// Collect one second of data while holding at the starting position.
			apparatus->Wait( duration );
		}
		
		if ( !strcmp( token, "TargetsOff" ) ) {
			apparatus->TargetsOff();
		}
		
		if ( !strcmp( token, "TargetOn" ) ) {
			
			int target = 0;
			sscanf( line, "%s %d", token, &target );
			// Light up the next target.
			apparatus->TargetOn( target );
			
		}
		
		if ( !strcmp( token, "StopAcquisition" ) ) {
			// Stop collecting data.
			apparatus->StopAcquisition();
		}
		
		if ( !strcmp( token, "SaveAcquisition" ) ) {
			char tag[256];
			sscanf( line, "%s %s", token, tag );
			// Save the data.
			apparatus->SaveAcquisition( tag );
		}
		
		if ( !strcmp( token, "CheckVisibility" ) ) {
			
			double cumulative, continuous;
			sscanf( line, "%s %lf %lf", token, &cumulative, &continuous );
			
			// Check the quality of the data.
			status = apparatus->CheckVisibility( cumulative, continuous, NULL );
			if ( status == ABORT_EXIT ) exit( status );
			if ( status == RETRY_EXIT ) return( status );
		}
		
		if ( !strcmp( token, "SignalNormalCompletion" ) ) {
			
			char *prompt = strpbrk( line, " \t" );
			if ( !prompt ) prompt = "Terminated normally.";
			
			// Indicate to the subject that they are done.
			status = apparatus->SignalNormalCompletion( prompt );
			if ( status == ABORT_EXIT ) exit( status );
			
		}
	}
	
	return( NORMAL_EXIT );
	
}