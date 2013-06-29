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

DexCompiler::DexCompiler( int n_vertical_targets, 
						  int n_horizontal_targets,
						  int n_codas, int n_markers,
						  int n_tones, int n_channels,	
						  char *filename ) {
	
	nVerticalTargets = n_vertical_targets;
	nHorizontalTargets = n_horizontal_targets;
	nTargets = n_vertical_targets + n_horizontal_targets;
	nCodas = n_codas;
	nMarkers = n_markers;
	nTones = n_tones;
	nChannels = n_channels;
	script_filename = filename;

}

void DexCompiler::Initialize( void ) {
	
	fp = fopen( script_filename, "w" );
	if ( !fp ) {
		char message[1024];
		sprintf( message, "Error opening script file for write:\n  %s", script_filename );
	}
	
}

void DexCompiler::CloseScript( void ) {
	fclose( fp );
}

/***************************************************************************/

int DexCompiler::WaitSubjectReady( const char *message ) {
	fprintf( fp, "WaitSubjectReady\t%s\n", message );
	return( 0 );
}

void DexCompiler::Wait( double duration ) {
	fprintf( fp, "Wait\t%.6f\n", duration );
}

int	 DexCompiler::WaitUntilAtTarget( int target_id, float tolerance[3], float hold_time, float timeout, char *msg  ) {
	fprintf( fp, "WaitUntilAtTarget\t%d\t%.6f\t%.6f\t%.6f\n", target_id, tolerance[X],  tolerance[Y],  tolerance[Z] );
	return( 0 );
}

int DexCompiler::SelectAndCheckConfiguration( int posture, int bar_position, int tapping ) {
	fprintf( fp, "SelectAndCheckConfiguration\t%d\t%d\t%d\n", posture, bar_position, tapping );
	return( 0 );
}

void DexCompiler::SetTargetState( unsigned long target_state ) {
	fprintf( fp, "SetTargetState\t%ul\n", target_state );
}

void DexCompiler::SetSoundState( int tone, int volume ) {
	fprintf( fp, "SetSoundState\t%d\t%d\n", tone, volume );
}

int DexCompiler::CheckVisibility(  double max_cumulative_dropout_time, double max_continuous_dropout_time ) {
	fprintf( fp, "CheckVisibility\t%f\t%f\n", max_cumulative_dropout_time, max_continuous_dropout_time );
	return( 0 );
}

void DexCompiler::StartAcquisition( void ) {
	fprintf( fp, "StartAcquisition\n" );
}

void DexCompiler::StopAcquisition( void ) {
	fprintf( fp, "StopAcquisition\n" );
}

void DexCompiler::SaveAcquisition( void ) {
	fprintf( fp, "StopAcquisition\n" );
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