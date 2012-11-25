/*********************************************************************************/
/*                                                                               */
/*                                 CodaObjects.h                                 */
/*                                                                               */
/*********************************************************************************/

#ifndef CodaObjectsH
#define CodaObjectsH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

/***************************************************************************/

/*
 * Working volume.
 */

class Workspace : public Assembly {

private:

protected:

public:

  double radius;

  Workspace( void );

};


/***************************************************************************/

/*
 * Upright Subject.
 */

class Subject : public Assembly {

private:

protected:

public:

	Assembly  *body;
	Extrusion *torso;
	Assembly  *legs;
	Extrusion *leg;
	Ellipsoid *head;
	
  Subject( void );

};



/***************************************************************************/

/*
 * CODA Bar.
 */

class CODA : public Assembly {

private:

protected:

public:

  Assembly *all; // This intermediate assembly lets us reorient the whole thing.

  Slab *body;
  Assembly *caps;
  Assembly *lenses;
  Frustrum *frustrum;
  Slab *power_led, *acquire_led;

  double baseline;
  double toein;
  double reach;
  double fov;


  CODA( void );
  bool WorkspaceInView( Workspace *ws );
	void PowerOn( bool yn );
	void Acquire( bool yn );

};



#endif