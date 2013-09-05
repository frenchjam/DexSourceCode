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
	
DexCompiler::DexCompiler(   DexTracker  *tracker,
						    DexTargets  *targets,
							DexSounds	*sounds,
							DexADC		*adc,
							char *filename ) {

	// Even though we will not use the actual hardware devices, we create instances of each 
	// hardware component when we create the compiler. This is just so that the hardware
	// parameters such as the number of vertical and horizontal targets are set according to the
	// choice of hardware components.

	this->tracker = tracker;
	this->targets = targets;
	this->sounds = sounds;
	this->adc = adc;

	script_filename = filename;

}

void DexCompiler::Initialize( void ) {
	
	
	nVerticalTargets = targets->nVerticalTargets;
	nHorizontalTargets = targets->nHorizontalTargets;
	nTargets = nVerticalTargets + nHorizontalTargets;
	// Create bit masks for the horizontal and vertical targets.
	verticalTargetMask = 0;
	for ( int i = 0; i < nVerticalTargets; i++ ) verticalTargetMask = ( verticalTargetMask << 1 ) + 1;
	horizontalTargetMask = 0;
	for ( i = 0; i < nHorizontalTargets; i++ ) horizontalTargetMask = ( horizontalTargetMask << 1 ) + 1;
	horizontalTargetMask = horizontalTargetMask << nVerticalTargets;

	nTones = sounds->nTones;

	nCodas = tracker->nCodas;
	nMarkers = tracker->nMarkers;

	nChannels = adc->nChannels;


	// Open a file into which we will write the script.
	fp = fopen( script_filename, "w" );
	if ( !fp ) {
		char message[1024];
		sprintf( message, "Error opening script file for write:\n  %s", script_filename );
	}

	// Start with all the targets off.
	TargetsOff();
	// Make sure that the sound is off.
	SoundOff();
	
}

void DexCompiler::CloseScript( void ) {
	fclose( fp );
}

void DexCompiler::Quit( void ) {

	CloseScript();

}

/***************************************************************************/

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

int DexCompiler::WaitSubjectReady( const char *message ) {
	char msg[2048];
	// Transform linebreaks into back slashes.
	// It remains to be seen if the DEX GUI can handle multi-line messages.
	// If so, how should newines be indicated in the message string?
	strncpy( msg, message, sizeof( msg ) );
	for ( int i = 0; i < strlen( msg ); i++ ) {
		if ( msg[i] == '\n' ) msg[i] = '\\';
	}
	// There is no provision yet for an image file, so just put an empty field.
	// I did not foresee a timeout for this command. I am setting the timeout 
	// to zero, hoping that it means an indefinite timeout.
	fprintf( fp, "CMD_WAIT_SUBJ_READY, \"%s\", \"%s\", %.0f\n", msg, "", 0 );
	return( NORMAL_EXIT );
}

void DexCompiler::Wait( double duration ) {
	fprintf( fp, "CMD_WAIT, %.0f\n", duration * 10.0 );
}

int DexCompiler::WaitUntilAtTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {

	fprintf( fp, "CMD_WAIT_MANIP_ATTARGET, 0x%04x, 0x%04x, %f, %f, %f, %f, %.0f, %.0f, %.0f, %.0f, %.0f, %.0f, \"%s\"\n",
		horizontalTargetBit( target_id ), verticalTargetBit( target_id ), 
		desired_orientation[X], desired_orientation[Y], desired_orientation[Z], desired_orientation[M], 
		position_tolerance[X], position_tolerance[Y], position_tolerance[Z], orientation_tolerance,
		hold_time, timeout, msg );
	return( NORMAL_EXIT );
}

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
	fprintf( fp, "CMD_WAIT_MANIP_GRIP, %f, %.0f, %f, \"%s\"\n", min_force, tolerance, timeout, msg );
	return( NORMAL_EXIT );
}


int DexCompiler::SelectAndCheckConfiguration( int posture, int bar_position, int tapping ) {
	char message[256];
	sprintf( message, "Posture: %s   Target Bar: %s",
		( posture ? "supine" : "seated" ),
		( bar_position ? "Right" : "Left" ));
	// I am putting 0 for the timeout, because I don't believe that this one should timeout.
	fprintf( fp, "CMD_CHK_HW_CONFIG, \"%s\", \"%s\", %d, %d, %d \n", message, "", posture, bar_position, 0 );
	return( NORMAL_EXIT );
}

void DexCompiler::SetTargetStateInternal( unsigned long target_state ) {
	// I assumed that targets will be set with a single 32-bit word.
	// Instead, DEX treats horizontal and vertical with separate 16-bit words.
	unsigned short vstate, hstate;
	vstate = verticalTargetBits( target_state );
	hstate = horizontalTargetBits( target_state );
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
	fprintf( fp, "CMD_CTRL_TONE, %d, %d\n", ( volume ? 1 : 0 ), tone );
}

int DexCompiler::CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg ) {
	fprintf( fp, "CMD_CHK_MANIP_VISIBILITY, %f, %f, \"%s\"\n", 
		max_cumulative_dropout_time, max_continuous_dropout_time, msg );
	return( NORMAL_EXIT );
}

// DEX does not implement overrun checks.
int DexCompiler::CheckOverrun(  const char *msg ) {
	return( NORMAL_EXIT );
}

int DexCompiler::CheckMovementAmplitude(  double min, double max, 
										   double dirX, double dirY, double dirZ,
										   const char *msg ) {
	fprintf( fp, "CMD_CHK_MOVEMENTS_AMPL, %.0f, %.0f, %f, %f, %f, \"%s\"\n", 
		min, max, dirX, dirY, dirZ, msg );
	return( NORMAL_EXIT );
}

void DexCompiler::StartAcquisition( const char *tag, float max_duration ) {
	// DEX ignores the max duration parameter.
	fprintf( fp, "CMD_ACQ_START, \"%s\"\n", tag ); 
	// Note the time of the acqisition start.
	MarkEvent( ACQUISITION_START );
}

int DexCompiler::StopAcquisition( const char *msg ) {
	MarkEvent( ACQUISITION_STOP );
	fprintf( fp, "CMD_ACQ_STOP, \"%s\"\n", msg );
	return( NORMAL_EXIT );
}

void DexCompiler::MarkEvent( int event, unsigned long param ) {
	fprintf( fp, "CMD_LOG_EVENT, %d\n", event );
}

int DexCompiler::CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, const char *msg ) {
	fprintf( fp, "CMD_CHK_CODA_ALIGNMENT, 0x%08lx, %.0f, %d,\"%s\"\n", marker_mask, tolerance, n_good, msg );
	return( NORMAL_EXIT );
}

// The following routines do not output anything to the scripts. 
// They represent stuff that cannot be executed by DEX.

void DexCompiler::UnhandledCommand( const char *cmd ) {
	char msg[2048];
	sprintf( msg, "Warning - Command is not handled by DEX script processor: %s", cmd );
	MessageBox( NULL, msg, "DEX Compiler Warning", MB_OK );
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

// It is normal to encounter this command in the task programs.
// But it should not generate a command in the script.
// I have it generate a comment for the moment.
void DexCompiler::SignalEvent( const char *event ) {
	fprintf( fp, "#SignalEvent, %s\n", event );
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