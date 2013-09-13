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
#include "DexTargets.h"
#include "Dexterous.h"

/***************************************************************************/

DexTargets::DexTargets( void ) {
	
	nTargets = nVerticalTargets = nHorizontalTargets = 0;
	
}


void DexTargets::Initialize( void ) {

	nTargets = nVerticalTargets + nHorizontalTargets;
	lastTargetOn = 0;

	AllTargetsOff();
	Update();

}

/*********************************************************************************/

// These default methods do nothing and must be overlayed by a derived class.

void DexTargets::SetTargetStateInternal( unsigned long state ) {}

/*********************************************************************************/

void DexTargets::SetTargetState( unsigned long bit_pattern ) {
	unsigned long bit;
	int i;
	targetState = bit_pattern;
	for ( i = 0, bit = 0x01; i < nTargets; i++, bit = bit << 1 ) {
		if ( bit & bit_pattern ) {
			lastTargetOn = i;
			break;
		}
	}
	SetTargetStateInternal( bit_pattern );
}

/*********************************************************************************/

void DexTargets::AllTargetsOff( void ) {
	SetTargetState( 0x00000000L );
}

void DexTargets::TurnTargetOn( int id ) { 
	
	unsigned long new_bit;
	
	lastTargetOn = id;
	new_bit = 0x00000001 << id;
	SetTargetState( targetState | new_bit );
}

unsigned long DexTargets::GetTargetState( bool target_state_array[DEX_MAX_TARGETS] ) {
	
	unsigned long bit;
	int i;
	
	if ( target_state_array != NULL ) {
		for ( i = 0, bit = 0x01; i < DEX_MAX_TARGETS; i++, bit = bit << 1 ) {
			target_state_array[i] = ( targetState & bit ? true : false );
		}
	}
	return( targetState );
}

int  DexTargets::GetLastTargetOn( void ) {
	return( lastTargetOn );
}

int  DexTargets::Update( void ) { 
	return(0); 
}
void DexTargets::Quit( void ) {}

/***************************************************************************/
/*                                                                         */
/*                             DexScreenTargets                            */
/*                                                                         */
/***************************************************************************/

DexScreenTargets::DexScreenTargets( HWND parent, int n_vertical, int n_horizontal ) {
		
	RECT rect;
	int width_in_pixels, height_in_pixels;
	double height, target_width = 25.0, target_thickness = 10.0;
	int trg, i;
	
	// If a parent window was not specified, use the desktop.
	if ( parent == NULL ) parent = GetDesktopWindow();

	// Create a window to display the virtual targets.
	GetWindowRect( parent, &rect );

	width_in_pixels = ( rect.right - rect.left ) / 20;
	height_in_pixels = rect.bottom - rect.top - 1;
	
	vertical_window = new OpenGLWindow();
	vertical_window->Border = false;
	vertical_window->Create( NULL, "DEX", rect.right - width_in_pixels, 0, width_in_pixels, height_in_pixels - 50 );
	vertical_viewpoint = new OrthoViewpoint( - width_in_pixels / 2, width_in_pixels / 2, 0, height_in_pixels, -100, 100 );	
	vertical_targets = new Assembly;

	height = height_in_pixels;
	for ( trg = 0; trg < n_vertical; trg++ ) {
				
		// The next are the positions of the targets in the target window on the screen.
		target[trg] = new Slab( target_width, target_thickness );
		target[trg]->SetPosition( 0.0, height - height / n_vertical * (trg + 0.5), 0.0 );
		target[trg]->SetColor( RED );
		vertical_targets->AddComponent( target[trg] );
		
	}

	// Now do the same as above for the horizontal target bar across the top of the screen.
	// Height of this bar is the same as the width of the vertical bar.
	height_in_pixels = width_in_pixels;
	// Width of this bar is full width of screen, minus enough space for the vertical bar at the right.
	width_in_pixels = ( rect.right - rect.left ) - 2 * width_in_pixels - 4;
	horizontal_window = new OpenGLWindow();
	horizontal_window->Border = false;
	horizontal_window->Create( NULL, "DEX", 0, 0, width_in_pixels, height_in_pixels );

	horizontal_viewpoint = new OrthoViewpoint( 0, width_in_pixels, - height_in_pixels / 2, height_in_pixels / 2, -100, 100 );	
	horizontal_targets = new Assembly;

	height = width_in_pixels;
	for ( trg = trg, i = 0; i < n_horizontal; trg++, i++ ) {
				
		// These are the positions of the targets in the target window on the screen.
		target[trg] = new Slab( target_width, target_thickness );
		target[trg]->SetPosition( height - height / n_horizontal * (i + 0.5), 0.0, 0.0 );
		
		target[trg]->SetOrientation( 90.0, k_vector );
		target[trg]->SetColor( RED );
		horizontal_targets->AddComponent( target[trg] );
		
	}

	nTargets = n_vertical + n_horizontal;
	nVerticalTargets = n_vertical;
	nHorizontalTargets = n_horizontal;
	
	Draw();

}

void DexScreenTargets::Initialize( void ) {}

/*********************************************************************************/

void DexScreenTargets::SetTargetStateInternal( unsigned long state ) {
	unsigned long bit;
	int trg;
	for ( trg = 0, bit = 0x01; trg < nTargets; trg++, bit = bit << 1 ) {
		if ( bit & state ) target[trg]->SetColor( RED );
		else target[trg]->SetColor( GRAY );
	}
}

/*********************************************************************************/

void DexScreenTargets::Draw( void ) {
	
	vertical_window->Activate();
	vertical_viewpoint->Apply( vertical_window, CYCLOPS );
	vertical_window->Clear( BLUE );
	vertical_targets->Draw();
	vertical_window->Swap();

	horizontal_window->Activate();
	horizontal_viewpoint->Apply( horizontal_window, CYCLOPS );
	horizontal_window->Clear( BLUE );
	horizontal_targets->Draw();
	horizontal_window->Swap();

}

int DexScreenTargets::Update( void ) {
	
	int i = 0;
	int exit_status = 0;
	int input = 0;
	
	input = vertical_window->GetInput( 0.1 );
	if ( input == WM_QUIT ) {
		exit_status = ESCAPE_EXIT;
	}
	Draw();
	return( exit_status );
	
}


