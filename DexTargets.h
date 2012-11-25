/*********************************************************************************/
/*                                                                               */
/*                                  DexTargets.h                                 */
/*                                                                               */
/*********************************************************************************/

/*
* Objects that run a set of targets<.
*/

#ifndef DexTargetH
#define DexTargetH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include "Dexterous.h"

class DexTargets {
	
private:
	
protected:
	
public:
	
	int				nTargets;
	int				lastTargetOn;
	unsigned long	targetState;
	
	virtual void SetTargetStateInternal( unsigned long target_bit_pattern );
	virtual int  Update( void );
	virtual void Quit( void );
	
	// Set a pattern of targets.
	void SetTargetState( unsigned long target_bit_pattern );
	// Turn on 1 target.
	void TurnTargetOn( int n );
	// Turn off all the targets.
	void AllTargetsOff( void );

	// Retrieve the ID of the last target that was turned on.
	// If more than one was turned on, it will return the lowest ID.
	int GetLastTargetOn( void );
	// Retrieve a list of yes/no values indicating the state of each LED.
	unsigned long GetTargetState( bool target_state_array[DEX_MAX_TARGETS] = NULL );
	
	DexTargets( int n_vertical = N_VERTICAL_TARGETS, int n_horizontal = N_HORIZONTAL_TARGETS );
	
};

class DexScreenTargets : public DexTargets {
	
private:
	
	// OpenGL objects representing the CODA bars and target frame.	
	OpenGLWindow			*vertical_window, *horizontal_window;
	OrthoViewpoint			*vertical_viewpoint, *horizontal_viewpoint;
	Assembly				*vertical_targets, *horizontal_targets;
	Slab					*target[DEX_MAX_TARGETS];
	
	void Draw( void );
	
	
protected:
	
public:
	
	DexScreenTargets( int n_vertical = N_VERTICAL_TARGETS, int n_horizontal = N_HORIZONTAL_TARGETS );
	

	void SetTargetStateInternal( unsigned long target_bit_pattern );
	int  Update( void );
	
};

#endif