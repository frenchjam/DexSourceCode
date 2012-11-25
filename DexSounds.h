/*********************************************************************************/
/*                                                                               */
/*                                    DexSounds.h                                */
/*                                                                               */
/*********************************************************************************/

/*
* Objects that run sound generation.
*/

#ifndef DexSoundH
#define DexSoundH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include "Dexterous.h"

class DexSounds {
	
private:

protected:

	bool	mute;
	int		hold_volume;
	int		hold_tone;
	int		nTones;

	// The work horse. Must be provided by derived class.
	// Will be invoked by the public method SetSoundState().
	virtual void SetSoundStateInternal( int tone, int volume );

public:
	
	// Constructor.
	DexSounds( int tones = N_TONES );

	// These functions are all defined by the base class. It is unlikely that
	// they will be replaced by the derived classes, but they are defined
	// as virtual just in case.

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

#endif