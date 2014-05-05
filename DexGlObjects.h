/*********************************************************************************/
/*                                                                               */
/*                                  DexGlObjects.h                               */
/*                                                                               */
/*********************************************************************************/

/* 
* Objects used in the virtual scene for the DEX apparatus simulator.
*/

#ifndef DexGlObjectsH
#define DexGlObjectsH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include "Dexterous.h"

#define MAX_LEDS 25
#define LED_RADIUS 8
#define BAR_THICKNESS 20

/***************************************************************************/

/*
* DEX Target Bar.
*/

class DexVirtualTargetBar : public Assembly {
	
private:
	
protected:
	
public:
	
	int nTargets;

	Assembly	*bar;
	Assembly	*box;
	Sphere		*led[MAX_LEDS];
	
	Slab		*upper_tap, *lower_tap;
	
	DexVirtualTargetBar( double length = 300, int vertical_targets = N_VERTICAL_TARGETS, int horizontal_targets = N_HORIZONTAL_TARGETS );
	
	void SetTargetState( unsigned long target_state );
	void TargetOn( int n );
	void TargetOff( int n );
	void TargetsOff( void );
	
	void SetConfiguration( DexSubjectPosture posture,
							DexTargetBarConfiguration bar,
							DexTappingSurfaceConfiguration tapping );
	
};



#endif