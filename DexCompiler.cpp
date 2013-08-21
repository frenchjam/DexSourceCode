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

int DexCompiler::WaitSubjectReady( const char *message ) {
	fprintf( fp, "WaitSubjectReady\t%s\n", message );
	return( NORMAL_EXIT );
}

void DexCompiler::Wait( double duration ) {
	fprintf( fp, "Wait\t%.6f\n", duration );
}

int DexCompiler::WaitUntilAtTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg  ) {
	fprintf( fp, "WaitUntilAtTarget, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, \"%s\"\n",
		target_id, 
		desired_orientation[X], desired_orientation[Y], desired_orientation[Z], desired_orientation[M], 
		position_tolerance[X], position_tolerance[Y], position_tolerance[Z], orientation_tolerance,
		hold_time, timeout, msg );
	return( NORMAL_EXIT );
}

int	 DexCompiler::WaitCenteredGrip( float tolerance, float min_force, float timeout, char *msg ) {
	fprintf( fp, "WaitCenteredGrip, %f, %f, %f, \"%s\"\n", tolerance, min_force, timeout, msg );
	return( NORMAL_EXIT );
}


int DexCompiler::SelectAndCheckConfiguration( int posture, int bar_position, int tapping ) {
	fprintf( fp, "SelectAndCheckConfiguration\t%d\t%d\t%d\n", posture, bar_position, tapping );
	return( NORMAL_EXIT );
}

void DexCompiler::SetTargetState( unsigned long target_state ) {
	fprintf( fp, "SetTargetState\t%ul\n", target_state );
}

void DexCompiler::SetSoundState( int tone, int volume ) {
	fprintf( fp, "SetSoundState\t%d\t%d\n", tone, volume );
}

int DexCompiler::CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg ) {
	fprintf( fp, "CheckVisibility, %f, %f, \"%s\"\n", max_cumulative_dropout_time, max_continuous_dropout_time, msg );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckOverrun(  const char *msg ) {
	fprintf( fp, "CheckOverrun, \"%s\"\n", msg );
	return( NORMAL_EXIT );
}

int DexCompiler::CheckMovementAmplitude(  double min, double max, 
										   double dirX, double dirY, double dirZ,
										   const char *msg ) {
	fprintf( fp, "CheckMovementAmplitude, %f, %f, %f, %f, %f, \"%s\"\n", 
		min, max, dirX, dirY, dirZ, msg );
	return( NORMAL_EXIT );
}

void DexCompiler::StartAcquisition( float max_duration ) {
	fprintf( fp, "StartAcquisition\n" );
	// Note the time of the acqisition start.
	MarkEvent( ACQUISITION_START );
}

void DexCompiler::StopAcquisition( void ) {
	MarkEvent( ACQUISITION_STOP );
	fprintf( fp, "StopAcquisition\n" );
}

void DexCompiler::SaveAcquisition( const char *tag ) {
	MarkEvent( ACQUISITION_SAVE );
	fprintf( fp, "SaveAcquisition\n" );
}

void DexCompiler::MarkEvent( int event, unsigned long param ) {
	fprintf( fp, "CMD_LOG_EVENT,%d\n", event );
}

int DexCompiler::CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, const char *msg ) {
	fprintf( fp, "CheckTrackerAlignment,%ul,%f,%d,\"%s\"\n", marker_mask, tolerance, n_good, msg );
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
			sscanf( line, "%s %f", token, &max_duration );
			// Start acquiring data.
			apparatus->StartAcquisition( max_duration );
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