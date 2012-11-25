/*********************************************************************************/
/*                                                                               */
/*                                 DexGlObjects.cpp                              */
/*                                                                               */
/*********************************************************************************/

/* 
 * Objects used in the virtual scene for the DEX apparatus simulator.
 */

#include <stdio.h>
#include <windows.h>


#include <gl/gl.h>
#include <gl/glu.h>

#include <useful.h>
#include <3dMatrix.h>
#include <Trackers.h>

#include <OpenGLWindows.h>
#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include "DexGlObjects.h"

/***************************************************************************/

DexVirtualTargetBar::DexVirtualTargetBar( double length, int vertical_targets, int horizontal_targets ) {

	Slab *slab;
	
	nTargets = vertical_targets + horizontal_targets;
	double spacing;
	int i, j;

	// Create the vertical target bar.
	bar = new Assembly();		
	slab =	new Slab( BAR_THICKNESS, length, BAR_THICKNESS );
	slab->SetColor( GRAY );
	slab->SetOffset( 0.0, length / 2.0, 0.0 );
	bar->AddComponent( slab );

	// The box.
	box = new Assembly();
	slab = new Slab( 120.0, 50.0, length );
	slab->SetColor( GRAY );
	slab->SetOffset( 0.0, -25.0, -length / 2 );
	box->AddComponent( slab );

	// Targets on the vertical bar.
	spacing = length / (vertical_targets + 1.0);
	for ( i = 0; i < vertical_targets; i ++ ) {
		led[i] = new Sphere( LED_RADIUS );
		led[i]->SetPosition( 0.0,  spacing * ( i + 1), BAR_THICKNESS );
		led[i]->SetColor( GREEN );
		bar->AddComponent( led[i] );
	}

	// Now add the targets that are on the box.
	spacing = length / (horizontal_targets + 1.0);
	for ( i = i, j = 0; j < horizontal_targets; i ++, j++ ) {
		led[i] = new Sphere( LED_RADIUS );
		led[i]->SetPosition( 60.0, 0.0, - spacing * ( j + 1 ) );
		led[i]->SetColor( GREEN );
		box->AddComponent( led[i] );
	}

	upper_tap = new Slab( BAR_THICKNESS, spacing, BAR_THICKNESS );
	upper_tap->SetColor( ORANGE );
	upper_tap->SetOffset( 0.0, -3.0 * spacing / 4.0, - BAR_THICKNESS );
	upper_tap->SetPosition( 0.0, length - spacing / 4.0, 0.0 );
	bar->AddComponent( upper_tap );

	lower_tap = new Slab( BAR_THICKNESS, spacing, BAR_THICKNESS );
	lower_tap->SetColor( ORANGE );
	lower_tap->SetOffset( 0.0, 3.0 * spacing / 4.0, - BAR_THICKNESS );
	lower_tap->SetPosition( 0.0, spacing / 4.0, 0.0 );
	bar->AddComponent( lower_tap );

	AddComponent( bar );
	AddComponent( box );

	TargetsOff();

	SetConfiguration( PostureSeated, TargetBarLeft, TappingExtended );
}

/***************************************************************************/

void DexVirtualTargetBar::SetTargetState( unsigned long target_state ) {

	unsigned long bit;
	int i;

	for ( i = 0, bit = 0x01; i < nTargets; i++, bit = bit << 1 ) {
		if ( target_state & bit ) led[ i ]->SetColor( GREEN );
		else led[ i ]->SetColor( BLUE );
	}
}

void DexVirtualTargetBar::TargetOn( int target ) {
	if ( target < 0 || target >= nTargets ) return;
	led[ target ]->SetColor( GREEN );
}

void DexVirtualTargetBar::TargetOff( int target ) {
	if ( target < 0 || target >= nTargets ) return;
	led[ target ]->SetColor( BLUE );
}

void DexVirtualTargetBar::TargetsOff( void ) {
	for ( int i = 0; i < nTargets; i++ ) led[i]->SetColor( BLUE );
}

/***************************************************************************/

void DexVirtualTargetBar::SetConfiguration( DexSubjectPosture posture,
											DexTargetBarConfiguration bar_config,
											DexTappingSurfaceConfiguration tapping ) {

	if ( posture == PostureSupine ) {
		SetOrientation( 90.0, i_vector );
	}
	else {
		SetOrientation( 0.0, i_vector );
	}

	switch ( bar_config ) {

		case TargetBarRight:
			bar->SetPosition( 40.0, 0.0, -100.0 );
			bar->SetOrientation( 0.0, i_vector );
			break;

		case TargetBarLeft:
			bar->SetPosition( -40.0, 0.0, -100.0 );
			bar->SetOrientation( 0.0, i_vector );
			break;

	}

	if ( tapping == TappingExtended ) {

		if ( bar_config == TargetBarLeft ) {
			upper_tap->Enable();
			lower_tap->Enable();
			upper_tap->SetOrientation(  90.0, k_vector );
			lower_tap->SetOrientation( -90.0, k_vector );
		}
		else {
			upper_tap->Enable();
			lower_tap->Enable();
			upper_tap->SetOrientation( -90.0, k_vector );
			lower_tap->SetOrientation(  90.0, k_vector );
		}

	}
	else {
		upper_tap->Disable();
		lower_tap->Disable();
		upper_tap->SetOrientation( 0.0, k_vector );
		lower_tap->SetOrientation( 0.0, k_vector );
	}



}

