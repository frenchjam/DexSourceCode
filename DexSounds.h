/*********************************************************************************/
/*                                                                               */
/*                                    DexSounds.h                                */
/*                                                                               */
/*********************************************************************************/

/*
* Objects that run sound generation.
*/

#pragma once

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include <NIDAQmx.h>

#include "Dexterous.h"

class DexSounds {
	
private:

protected:

	bool	mute;
	int		hold_volume_for_mute;
	int		hold_tone_for_mute;

	// The work horse. Must be provided by derived class.
	// Will be invoked by the public method SetSoundState().
	virtual void SetSoundStateInternal( int tone, int volume );

public:

	// The number of tones that this device can produce.
	int		nTones;

	// Constructor.
	DexSounds( void );

	// These functions are defaults defined by the base class. 
	// They may be replaced by the derived classes.

	// Any hardware initialization that might be needed.
	void Initialize( void );

	// Turns sounds on and off. Takes into account the mute flag
	//  and then calls SetSoundStateInternal().
	virtual void SetSoundState( int tone, int volume );

	// Will be called occasionally in case the system needs some sort
	// of periodic updating. If such updating is not needed, this default is fine.
	// A return of non-zero means that the application should terminate.
	virtual int Update( void );

	// Will be called when done. Again, redfining this may not be necessary.
	// The default behavior is just to call SoundOff().
	virtual void Quit( void );

	// By defaultm translates into turning all sounds off.
	virtual void SoundOff( void );

	// Allows one to ignore future sound commands.
	virtual void Mute( bool on_off );
	
};

/***************************************************************************/
/*                                                                         */
/*                             DexScreenSounds                             */
/*                                                                         */
/***************************************************************************/

// A graphic representation that allows us to test sound commands without making noise.

class DexScreenSounds : public DexSounds {
	
private:
	
	// OpenGL objects representing a speaker in the visual scene.
	OpenGLWindow			*window;
	Viewpoint				*viewpoint;
	Sphere					*speaker;
	
	void Draw( void );
	
protected:

	int Update( void );
	void SetSoundStateInternal( int tone, int volume );
	
public:
	
	DexScreenSounds( int tones = N_TONES );
		
};

/***************************************************************************/
/*                                                                         */
/*                           DexNiDaqAdcSounds                             */
/*                                                                         */
/***************************************************************************/

// Use an analog output to generate tones. Useful with GLMbox, because the
// GLMbox metronome uses digital output bits that we need for driving targets.

// Sampling frequency, in Hertz.
#define DEX_NIDAC_SOUND_SAMPLE_FREQUENCY		11000.0

// The following value determines the lowest frequency that can be played.
// Fmin = Fsample / Nsamples.
#define DEX_NIDAC_SOUND_BUFFER_MAX_SAMPLES	1100		

class DexNiDaqAdcSounds : public DexSounds {
	
private:
		
	void		ReportNiDaqError ( void );
	float64		buffer[N_TONES][DEX_NIDAC_SOUND_BUFFER_MAX_SAMPLES];
	int32		nidaq_samples[N_TONES];
	TaskHandle	taskHandle;	

	int last_tone;
	int last_volume;

protected:

	void SetSoundStateInternal( int tone, int volume );
	
public:
	
	DexNiDaqAdcSounds( int tones = N_TONES );
		
};


