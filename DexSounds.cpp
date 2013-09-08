/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

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

}

void DexSounds::Initialize( void ) {}

void DexSounds::SoundOff( void ) {
	SetSoundState( 0, 0 );
}
void DexSounds::Mute( bool on_off ) { 
	mute = on_off;
	if ( mute ) {
		SetSoundStateInternal( 0, 0 );
	}
	else {
		SetSoundStateInternal( hold_tone, hold_volume );
	}
}

void DexSounds::SetSoundState( int tone, int volume ) {

	hold_tone = tone;
	hold_volume = volume;

	if ( tone >= N_TONES ) tone = N_TONES - 1;
	if ( volume >= N_VOLUMES ) volume = N_VOLUMES - 1;
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
	int width_in_pixels, height_in_pixels;

	// The creator of this device specifies how many tones it can produce.
	nTones = tones;

	// Do whatever the base class would do first.
	DexSounds::DexSounds();

	// Create a window to display the virtual speaker.
	GetWindowRect( GetDesktopWindow(), &rect );
	width_in_pixels = ( rect.bottom - rect.top ) / 10;
	height_in_pixels = width_in_pixels + 48;

	window = new OpenGLWindow();
	// I would rather not have a border on this window, but not having
	// a border causes the mouse pointer to dissappear in the other 
	// windows as well.
	window->Border = true;
	window->Create( NULL, "DEX", rect.right - width_in_pixels - 200, rect.top + 120, width_in_pixels, height_in_pixels );

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

	static int tone_to_color[N_TONES] = { BLACK, MAGENTA, BLUE, CYAN, GREEN, YELLOW, ORANGE, RED };

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

	input = window->GetInput( 0.1 );
	if ( input == WM_QUIT ) exit_status = ESCAPE_EXIT;
	Draw();
	return( exit_status );

}

/*********************************************************************************/

