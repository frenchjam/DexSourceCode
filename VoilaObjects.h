/*********************************************************************************/
/*                                                                               */
/*                                VoilaObjects.h                                 */
/*                                                                               */
/*********************************************************************************/

#ifndef VoilaObjectsH
#define VoilaObjectsH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#define LED_RADIUS 15

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
 * A set of spherical markers showing the leds attached to a rigid body.
 */

class SensorStructure : public Assembly {

private:

protected:

public:

	SensorStructure( int n_markers = 3, double led_size = LED_RADIUS );
	SetLEDSize( double size );
	SensorSetLEDPositions( int sensor );
  SetLEDColor( int color );
	
	Sphere *led[ MAX_MARKERS ];
	int     leds;
  double  led_size;

};

/***************************************************************************/

/*
 * Chest Pack.
 */

class ChestPack : public SensorStructure {

private:

protected:

public:

  Slab  *box;
  ChestPack( int n_markers = 3, double led_size = LED_RADIUS );

};

/***************************************************************************/

/*
 * Back Pack.
 */

class BackPack : public SensorStructure {

private:

protected:

public:

  Slab  *plate;
  BackPack( int n_markers = 3, double led_size = LED_RADIUS );

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
 * Voila SRS platform.
 */

class SRS : public Assembly {

private:

protected:

public:

  Assembly *structure;

  Slab *platform;
  Slab *backrest;
  Slab *footrest;

  Bar *left_bar;
  Bar *right_bar;

  SRS( void );
  void __fastcall SetBackrestAngle( double angle );

};

/***************************************************************************/

/*
 * ISS Module.
 */

class ISSModule : public Assembly {

private:

protected:

public:

  Slab *floor;
  Slab *ceiling;
  Slab *back_wall;
  Slab *front_wall;

  Assembly *far_bulkhead;
  Assembly *near_bulkhead;

  Texture       *wall_texture;

  ISSModule( double length = 6000.0, double width = 2500.0, double height = 2500.0 );

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

/***************************************************************************/

/*
 * Wand.
 */

class Wand : public SensorStructure {

private:

protected:

public:

  Bar *blade;
  Sphere *hilt;

  Wand( void );

};

/***************************************************************************/

/*
 * HMD.
 */

class HMD : public SensorStructure {

private:

protected:

public:

  Assembly *body;
  Sphere *left_eyepiece;
  Sphere *right_eyepiece;

  HMD( void );

};

#endif