/*********************************************************************************/
/*                                                                               */
/*                                CodaObjects.cpp                                */
/*                                                                               */
/*********************************************************************************/


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

#include "CodaObjects.h"

static double default_workspace_radius = 1000.0;


/***************************************************************************/

CODA::CODA( void ) {
	
	double near_vertex[3], far_vertex[3];
	double vergence;
	
	
	// Initialize standard parameters.
	reach = 3000.0;
	baseline = 600.0;
	toein = 7.0;
	fov = 70.0;
	
	// Create a structure to hold everything together.
	all = new Assembly();
	AddComponent( all );
	
	// Compute frustrum.
	vergence = radians( fov / 2.0 + toein );
	
	near_vertex[Z] = sin( PI / 2.0 - vergence ) * baseline / sin( 2.0 * vergence );
	near_vertex[X] = 0.0;
	near_vertex[Y] = near_vertex[Z] * tan( radians( fov / 2.0 ) );
	
	far_vertex[X] = reach * cos( PI / 2.0 - vergence ) - baseline / 2.0;
	far_vertex[Y] = reach * sin( PI / 2.0 - vergence );
	far_vertex[Z] = reach * sin( radians( fov / 2.0 ) );
	
	frustrum = new Frustrum( near_vertex, far_vertex );
	frustrum->SetColor( Translucid( MAGENTA ) );
	all->AddComponent( frustrum );
	
	// Create the camera bar objects.
	
	Slab *slab;
	double length = baseline + 120;
	
	body = new Slab( length, 120.0, 100.0);
	//  body->SetColor( GRAY );
	all->AddComponent( body );
	
	caps = new Assembly();
	slab = new Slab( 40.0, 140.0, 120.0 );
	slab->SetPosition( length / 2.0 + 20.0, 0.0, 0.0 );
	caps->AddComponent( slab );
	slab = new Slab( 40.0, 140.0, 120.0 );
	slab->SetPosition( - length / 2.0 - 20.0, 0.0, 0.0 );
	caps->AddComponent( slab );
	caps->SetColor( BLACK );
	all->AddComponent( caps );
	
	lenses = new Assembly();
	all->AddComponent( lenses );
	lenses->SetColor( BLACK );
	
	slab = new Slab( 140.0, 80.0, 40.0 );
	slab->SetPosition( - baseline / 2.0 + 20, 0.0, 50.0 );
	lenses->AddComponent( slab );
	
	slab = new Slab( 140.0, 80.0, 40.0 );
	slab->SetPosition( baseline / 2.0 - 20, 0.0, 50.0 );
	lenses->AddComponent( slab );
	
	slab = new Slab( 120.0, 80.0, 40.0 );
	slab->SetPosition( 0.0, 0.0, 50.0 );
	lenses->AddComponent( slab );
	
	slab = new Slab( 100.0, 80.0, 40.0 );
	slab->SetPosition( - baseline / 4.0, 0.0, 50.0 );
	lenses->AddComponent( slab );

	power_led = new Slab( 20, 10, 10 );
	power_led->SetPosition( - baseline / 4.0 - 20, - 20.0, 80.0 );
	power_led->SetColor( GREEN );
	lenses->AddComponent( power_led );

	acquire_led = new Slab( 20, 10, 10 );
	acquire_led->SetPosition( - baseline / 4.0 - 20, 20.0, 80.0 );
	acquire_led->SetColor( ORANGE );
	lenses->AddComponent( acquire_led );

	
	all->SetOrientation( 0.0, 90.0, 180.0 );
	
}

void CODA::PowerOn( bool yn ) {
	if ( yn ) power_led->SetColor( GREEN );
	else {
		power_led->SetColor( GRAY );
	}
}

void CODA::Acquire( bool yn ) {
	if ( yn ) acquire_led->SetColor( ORANGE );
	else {
		acquire_led->SetColor( GRAY );
	}
}

bool CODA::WorkspaceInView( Workspace *ws ) {
	
	double intrinsic[3], normal[3];
	
	/* B camera */
	intrinsic[X] = 0.0;
	intrinsic[Y] =   sin( radians( fov / 2.0 ) );
	intrinsic[Z] = - cos( radians( fov / 2.0 ) );
	premultiply_vector( orientation, intrinsic, normal );
	if ( distance_to_plane( ws->position, normal, position ) < ws->radius ) return( false );
	
	intrinsic[X] = 0.0;
	intrinsic[Y] =  - sin( - radians( fov / 2.0 ) );
	intrinsic[Z] =    cos( - radians( fov / 2.0 ) );
	premultiply_vector( orientation, intrinsic, normal );
	if ( distance_to_plane( ws->position, normal, position ) < ws->radius ) return( false );
	
	
	return( true );
	
}

