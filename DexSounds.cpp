/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <NIDAQmx.h>
#include <fMessageBox.h>

#include "VectorsMixin.h"  // This has to be before 3dMatrix.h.

#include <useful.h>
#include <screen.h>
#include <3dMatrix.h>
#include <Timers.h>
#include "ConfigParser.h" 

#include <CodaUtilities.h>

#include <gl/gl.h>
#include <gl/glu.h>

#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

#include "AfdObjects.h"
#include "CodaObjects.h"
#include "DexGlObjects.h"
#include "DexSounds.h"
#include "Dexterous.h"

/***************************************************************************/

DexSounds::DexSounds( void ) {

	nTones = 0;
	mute = false;
	hold_tone_for_mute = 0;

}

void DexSounds::Initialize( void ) {}

void DexSounds::SoundOff( void ) {
	SetSoundState( hold_tone_for_mute, 0 );
}
void DexSounds::Mute( bool on_off ) { 
	mute = on_off;
	if ( mute ) {
		SetSoundStateInternal( hold_tone_for_mute, 0 );
	}
	else {
		SetSoundStateInternal( hold_tone_for_mute, hold_volume_for_mute );
	}
}

void DexSounds::SetSoundState( int tone, int volume ) {


	if ( tone >= N_TONES ) tone = N_TONES - 1;
	if ( volume >= N_VOLUMES ) volume = N_VOLUMES - 1;
	hold_tone_for_mute = tone;
	hold_volume_for_mute = volume;
	if ( !mute ) SetSoundStateInternal( tone, volume );

}

// Default method. Does nothing.
void DexSounds::SetSoundStateInternal( int tone, int volume ) {}

// Default method. Does nothing.
int DexSounds::Update( void ) { return 0; } 

// Default method. Just be sure that sounds are off.
void DexSounds::Quit( void ) {	SoundOff(); }


/***************************************************************************/
/*                                                                         */
/*                             DexScreenSounds                             */
/*                                                                         */
/***************************************************************************/

DexScreenSounds::DexScreenSounds( int tones ) {


	RECT rect;
	int width_in_pixels;

	// The creator of this device specifies how many tones it can produce.
	nTones = tones;

	// Do whatever the base class would do first.
	DexSounds::DexSounds();

	// Create a window to display the virtual speaker.
	GetWindowRect( GetDesktopWindow(), &rect );
	width_in_pixels = ( rect.right - rect.left ) / 20;

	window = new OpenGLWindow();
	window->Border = false;
	window->Create( NULL, "DEX", rect.right - 2 * width_in_pixels - 2, rect.top, width_in_pixels, width_in_pixels );

	viewpoint = new Viewpoint();
	speaker = new Sphere();
	speaker->SetColor( RED );
	speaker->SetPosition( 0.0, 0.0, -50.0 );

	SoundOff();
	Draw();

}

/*********************************************************************************/

// This actually turns sounds on and off. 
// Here we fake it by blinking a indicator on and off.
// The size indicates the volume and the color the tone.

void DexScreenSounds::SetSoundStateInternal( int tone, int volume ) {

	static int tone_to_color[N_TONES] = { BLACK, MAGENTA, CYAN, GREEN, YELLOW, ORANGE, RED, WHITE };

	speaker->SetRadius( volume + 0.5 );
	speaker->SetColor( tone_to_color[tone] );

}

/*********************************************************************************/

void DexScreenSounds::Draw( void ) {
	
	window->Activate();
	viewpoint->Apply( window, CYCLOPS );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	window->Clear( BLUE );
	speaker->Draw();
	
	window->Swap();
	
}

int DexScreenSounds::Update( void ) {

	int i = 0;
	int exit_status = 0;
	int input = 0;

//	input = window->GetInput( 0.001 );
//	if ( input == WM_QUIT ) exit_status = ESCAPE_EXIT;
	Draw();
	return( exit_status );

}

/***************************************************************************/
/*                                                                         */
/*                           DexNidaqAdcSounds                             */
/*                                                                         */
/***************************************************************************/

// Use an analog output to generate tones. Useful with GLMbox, because the
// GLMbox metronome uses digital output bits that we need for driving targets.

DexNiDaqAdcSounds::DexNiDaqAdcSounds( int tones ) {

	// The creator of this device specifies how many tones it can produce.
	nTones = tones;

	// Do whatever the base class would do first.
	DexSounds::DexSounds();
	
	char channel_range[32];
	int32 error_code;

	int i, tone;
	double volume;
	char *taskname = "";

	// Fill the buffers with a single cycle of a sine wave at different frequencies.
	// Attenuate the amplitude as the frequency goes up to give a more-or-less even volume.
	double freq[N_TONES] = { 220.000, 261.626, 329.628, 391.995, 2 * 220.000, 2 * 261.626, 2 * 329.628, 2 * 391.995  };
	for ( tone = 0, volume = 0.1; tone < N_TONES; tone++, volume *= .8 ) {
		nidaq_samples[tone] = floor( DEX_NIDAC_SOUND_SAMPLE_FREQUENCY / freq[tone] );
		for ( i = 0; i < nidaq_samples[tone]; i++ ) {
			double theta = (double) i * PI * 2.0 / (double) nidaq_samples[tone];
			buffer[tone][i] = volume * sin( theta );
		}
	}

	// Define the analog channel onto which we will output sine waves.
	sprintf( channel_range, "Dev1/ao1" );

	// Initialize the ports for analog output.
	error_code = DAQmxCreateTask( taskname, &taskHandle );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Set to treat the single hardware channel as a single virtual channel.
	error_code = DAQmxCreateAOVoltageChan( taskHandle, channel_range, "", -5.0, 5.0, DAQmx_Val_Volts, NULL );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Set a clock to output the buffered samples.
	error_code = DAQmxCfgSampClkTiming( taskHandle, "OnboardClock", DEX_NIDAC_SOUND_SAMPLE_FREQUENCY, DAQmx_Val_Rising, DAQmx_Val_ContSamps, DEX_NIDAC_SOUND_BUFFER_MAX_SAMPLES );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();

	last_tone = -1;
	last_volume = -1;

}

/*********************************************************************************/

// This actually turns sounds on and off. 

void DexNiDaqAdcSounds::SetSoundStateInternal( int tone, int volume ) {

	int32 error_code;
	int32 samples;

	if ( tone != last_tone || volume != last_volume ) {
	error_code = DAQmxStopTask( taskHandle );
		if( DAQmxFailed( error_code ) ) ReportNiDaqError();
		if ( volume ) {
			error_code = DAQmxWriteAnalogF64 ( taskHandle, nidaq_samples[tone], 0, 0.01, DAQmx_Val_GroupByChannel, buffer[tone % nTones], &samples, 0);
			if( DAQmxFailed( error_code ) ) ReportNiDaqError();
			error_code = DAQmxStartTask( taskHandle );
			if( DAQmxFailed( error_code ) ) ReportNiDaqError();
		}

		last_tone = tone;
		last_volume = volume;
	}
	
}

void DexNiDaqAdcSounds::ReportNiDaqError ( void ) {
	char errBuff[2048]={'\0'};
	DAQmxGetExtendedErrorInfo( errBuff, 2048);
	fMessageBox( MB_OK, "DexNiDaqADC", errBuff );
}

