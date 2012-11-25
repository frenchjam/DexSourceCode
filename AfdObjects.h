#include <gl/gl.h>
#include <gl/glu.h>

#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

extern char AfdTexturePath[1024];

/********************************************************************************/

typedef struct {

  Texture *front;
  Texture *rear;
  Texture *left;
  Texture *right;
  Texture *ceiling;
  Texture *floor;

} RoomTextures;

/********************************************************************************/

class Dial : public Assembly {

private:

  Slab          *needle;

  double thickness;
  double radius;
  double tilt;
  double label_width;

  Texture *ten_texture;
  Texture *five_texture;
  Texture *zero_texture;
  Texture *meters_texture;

protected:

public:

  Dial( void );
  ~Dial( void );

  void __fastcall Draw( void );

  double minimum;
  double maximum;

  void SetValue( double value );

};

/********************************************************************************/

class HeightIndicator : public Assembly {

private:

protected:

public:

  HeightIndicator( void );
  ~HeightIndicator( void );

  void SetHeight( double height );

};

/********************************************************************************/

class MeterStick : public Assembly {

private:

protected:

public:

  MeterStick( void );
  ~MeterStick( void );

};

/********************************************************************************/

class Aperture : public Assembly {

private:

protected:

public:

  Aperture( void );
  Aperture( double room_width, double room_height );
  ~Aperture( void );

  double room_width;
  double room_height;
  double aperture;

  virtual void SetAperture ( double value );

};

class SlidingDoors : public Aperture {

private:

  Assembly *left_panel;
  Assembly *right_panel;

protected:

public:

  SlidingDoors( double room_width, double room_height, double thickness = 50.0 );
  ~SlidingDoors( void );
  void SetAperture ( double value );

};

class Hublot : public Aperture {

private:

  Hole *bulkhead;
  Cylinder *edge1, *edge2;
  Disk *border;

protected:

public:

  Hublot( double room_width, double room_height, double thickness = 50.0 );
  ~Hublot( void );
  void SetAperture ( double value );

};

/********************************************************************************/

typedef enum { SLIDING, HUBLOT } ApertureType;

class AfdEnvironment : public Assembly {

private:

protected:

public:

  double room_width;
  double room_height; 
  double room_length;
  double wall_thickness;
  double ceiling_height;
  double floor_height;

  AfdEnvironment( void );
  ~AfdEnvironment( void );

  // Viewing conditions.
  double viewing_distance;
  double viewing_height;

	bool bar_with_viewer;

  // State of the stimulus.
  double aperture;
  double perceived_eh;
  double perceived_distance;

  Viewpoint     *viewpoint;

  Aperture      *doorway;
  Dial          *dial;
  MeterStick    *meter_stick;
  Assembly      *indicator;

  Assembly      *right_wall, *left_wall;
  Slab          *deck;
  Slab          *ceiling;
  Patch         *backdrop;

  void SetViewingHeight ( double height );
  double SetViewingDistance ( double distance );

  void SetCeilingHeight ( double height );
  void SetFloorHeight ( double height );

  double SetAperture ( double value );
  double SetPerceivedEH ( double value );
  double SetPerceivedDistance ( double value );

};

/********************************************************************************/


class  ISS : public AfdEnvironment {

private:

protected:

public:

  ISS( ApertureType type = SLIDING, RoomTextures *tex = NULL, double width = 2000.0, double height = 2000.0, double length = 8000.0 );
  ~ISS( void );

  // Object dimensions in the virtual world.
  // All values are in millimeters.

  double rack_height; // Real height of Columbus module walls (rack) in mm.
  double rack_width; // Real width of rack in mm.
  
};

/********************************************************************************/

class  AfdRoom : public AfdEnvironment {

private:

protected:

public:

  AfdRoom( ApertureType type = SLIDING, RoomTextures *tex = NULL, double width = 2000.0, double height = 2000.0, double length = 2000.0 );
  ~AfdRoom( void );
  
};
