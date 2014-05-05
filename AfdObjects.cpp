/*********************************************************************************/
/*                                                                               */
/*                                 AfdObjects.cpp                                */
/*                                                                               */
/*********************************************************************************/

// Defines a set of OpenGLObjects for the Affordances experiment.
// We use this in the DEX experiment to get the ISS Columbus environment.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 

#include <useful.h>
#include <screen.h>
#include <3dMatrix.h>
#include <Timers.h>
#include "ConfigParser.h" 

#include <gl/gl.h>
#include <gl/glu.h>

#include "OpenGLUseful.h"
#include "OpenGLColors.h"
#include "OpenGLWindows.h"
#include "OpenGLObjects.h"
#include "OpenGLViewpoints.h"
#include "OpenGLTextures.h"

#include "AfdObjects.h"

char AfdTexturePath[1024] = ".\\";

GLfloat bulkhead_color[4] = { 0.33, 0.33, 0.66, 1.0 };
GLfloat cursor_color[4] = { 0.0, 1.0, 0.0, 1.0 };

/********************************************************************************/

Dial::Dial ( void ) {

  thickness = 10.0;
  radius = 100.0;
  tilt = 30.0;
  label_width = 50.0;

  Slab *slab;
  Disk *disk;
  Cylinder *cylinder;
  Patch *rectangle;
	char path[1024];

	sprintf( path, "%s%s", AfdTexturePath, "bmp\\ten.bmp" );
  ten_texture = new Texture( path, label_width / 2.0, label_width / 2.0 );
	sprintf( path, "%s%s", AfdTexturePath, "bmp\\five.bmp" );
  five_texture = new Texture( path, label_width / 2.0, label_width / 2.0 );
	sprintf( path, "%s%s", AfdTexturePath, "bmp\\zero.bmp" );
  zero_texture = new Texture( path, label_width / 2.0, label_width / 2.0 );
	sprintf( path, "%s%s", AfdTexturePath, "bmp\\meters.bmp" );
  meters_texture = new Texture( path, label_width * 2.0, label_width / 2.0 );
  
  needle = new Slab( thickness / 4.0, 0.90 * radius, thickness / 2.0 );
  needle->SetOffset( 0.0, radius / 2.0, thickness / 2.0 );
  needle->SetColor( ORANGE );
  AddComponent( needle );

  rectangle = new Patch( label_width / 2.0 , label_width / 2.0 );
  rectangle->SetPosition( radius + label_width / 2.0, 0.0, 0.0 );
  rectangle->SetTexture( ten_texture );
  AddComponent( rectangle );

  rectangle = new Patch( label_width / 2.0 , label_width / 2.0 );
  rectangle->SetPosition( 0.0, radius + label_width / 2.0, 0.0 );
  rectangle->SetTexture( five_texture );
  AddComponent( rectangle );

  rectangle = new Patch( label_width / 2.0 , label_width / 2.0 );
  rectangle->SetPosition( - ( radius + label_width / 2.0 ), 0.0, 0.0 );
  rectangle->SetTexture( zero_texture );
  AddComponent( rectangle );

  rectangle = new Patch( 2 * label_width , label_width / 2.0 );
  rectangle->SetPosition( 0.0, - radius - label_width / 2.0, 0.0 );
  rectangle->SetTexture( meters_texture );
//  AddComponent( rectangle );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( -18.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( -36.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( -54.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( -72.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( -90.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( 18.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( 36.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( 54.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( 72.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  slab = new Slab( thickness / 6.0, radius / 5.0, thickness / 4.0 );
  slab->SetOffset( 0.0, radius / 2.0 + radius * 4.0 / 10.0, thickness / 2.0 );
  slab->SetOrientation( 90.0, k_vector );
  slab->SetColor( BLACK );
  AddComponent( slab );

  disk = new Disk( radius );
  AddComponent( disk );
  cylinder = new Cylinder( radius, radius, thickness );
  AddComponent( cylinder );
  
  SetColor( CYAN );
  SetAttitude( tilt, i_vector );

  minimum = 0.0;
  maximum = 10000;

}

void Dial::Draw ( void ) {
  Assembly::Draw();
}

void Dial::SetValue ( double value ) {
  double angle = 180.0 * ((value - minimum ) / (maximum - minimum)) - 90.0;
  needle->SetOrientation( angle, k_vector ) ;
}

/*******************************************************************************/


HeightIndicator::HeightIndicator ( void ) {

  // Dimensions of the bar used to measure apparent eye height.
  double indicator_width = 400.0;
  double indicator_thickness = 40.0;
  double indicator_depth = 20.0;

  Slab *slab;

  slab = new Slab( indicator_width, indicator_thickness, indicator_depth );
  SetColor( cursor_color );
  AddComponent( slab );

}

void HeightIndicator::SetHeight ( double height ) {
  SetPosition( 0.0, height, 0.0 ) ;
}

/*******************************************************************************/

MeterStick::MeterStick( void ) {

  double stick_radius = 25.0;

  Cylinder *cylinder;
  Disk *disk;

  cylinder = new Cylinder( stick_radius, stick_radius, 200.0 );
  cylinder->SetColor( RED );
  AddComponent( cylinder );
  cylinder = new Cylinder( stick_radius, stick_radius, 200.0 );
  cylinder->SetOffset( 0.0, 0.0, 200.0 );
  cylinder->SetColor( WHITE );
  AddComponent( cylinder );
  cylinder = new Cylinder( stick_radius, stick_radius, 200.0 );
  cylinder->SetOffset( 0.0, 0.0, 400.0 );
  cylinder->SetColor( RED );
  AddComponent( cylinder );
  cylinder = new Cylinder( stick_radius, stick_radius, 200.0 );
  cylinder->SetOffset( 0.0, 0.0, 600.0 );
  cylinder->SetColor( WHITE );
  AddComponent( cylinder );
  cylinder = new Cylinder( stick_radius, stick_radius, 200.0 );
  cylinder->SetOffset( 0.0, 0.0, 800.0 );
  cylinder->SetColor( RED );
  AddComponent( cylinder );

  cylinder = new Cylinder( stick_radius / 2.0, stick_radius / 2.0, 100.0 );
  cylinder->SetAttitude( 90.0, j_vector );
  cylinder->SetColor( BLACK );
  AddComponent( cylinder );

  cylinder = new Cylinder( stick_radius / 2.0, stick_radius / 2.0, 100.0 );
  cylinder->SetAttitude( 90.0, j_vector );
  cylinder->SetPosition( 0.0, 0.0, 1000.0 - stick_radius );
  cylinder->SetColor( BLACK );
  AddComponent( cylinder );

  disk = new Disk( stick_radius );
  disk->SetPosition( 0.0, 0.0, 990.0 );
  disk->SetColor( RED );
  AddComponent( disk );

}

/*******************************************************************************/

Aperture::Aperture ( void ) {
  Aperture( 2000.0, 2000.0 );
}

Aperture::Aperture ( double room_width, double room_height ) {}

Aperture::~Aperture ( void ) {}

void Aperture::SetAperture ( double opening ) {

  aperture = opening;
  if ( aperture > room_width ) aperture = room_width;
  if ( aperture > room_height ) aperture = room_height;

}  

#define HANDLE_SIZE 30.0

SlidingDoors::SlidingDoors ( double room_width, double room_height, double thickness ) {

  Slab *slab;

  this->room_width = room_width;
  this->room_height = room_height;

  // Panels
  left_panel = new Assembly();
  slab = new Slab( room_width, room_height * 5.0, thickness );
  left_panel->AddComponent( slab );
  slab = new Slab( HANDLE_SIZE, room_height * 5.0, HANDLE_SIZE );
  slab->SetOffset( room_width / 2.0 - HANDLE_SIZE / 2.0, 0.0, thickness / 2.0 + HANDLE_SIZE );
  slab->SetColor( RED );
  left_panel->AddComponent( slab );
  left_panel->SetOffset( - room_width / 2.0, room_height / 2.0, 0.0 );
  AddComponent( left_panel );

  right_panel = new Assembly();
  slab = new Slab( room_width, room_height * 5.0, thickness );
  right_panel->AddComponent( slab );
  right_panel->SetOffset( room_width / 2, room_height / 2.0, 0.0 );
  slab = new Slab( HANDLE_SIZE, room_height * 5.0, HANDLE_SIZE );
  slab->SetOffset( - room_width / 2.0 + HANDLE_SIZE / 2.0, 0.0, thickness / 2.0 + HANDLE_SIZE );
  slab->SetColor( RED ); 
  right_panel->AddComponent( slab );
  AddComponent( right_panel );

  SetPosition( 0.0, 0.0, 0.0 );
  SetColor( bulkhead_color );

}

#define BORDER_THICKNESS  50.0
#define MAX_SHIFT 250.0
void SlidingDoors::SetAperture( double value ) {

  aperture = value;
  if ( aperture > room_width - 2 * BORDER_THICKNESS ) aperture = room_width - 2 * BORDER_THICKNESS;

  left_panel->SetPosition( - aperture / 2.0, 0.0, 0.0 );
  right_panel->SetPosition( aperture / 2.0, 0.0, 0.0 );

}

Hublot::Hublot ( double room_width, double room_height, double thickness ) {

  float rim_color[4] = { 0.8, 0.3, 0.3, 1.0 };
  float parois_color[4] = { 0.8, 0.3, 0.3, 1.0 };
  this->room_width = room_width;
  this->room_height = room_height;

  bulkhead = new Hole( 500.0, room_width, room_height * 5.0 );
  bulkhead->SetColor( bulkhead_color );
  AddComponent( bulkhead );

  edge1 = new Cylinder( 500.0, 500.0, thickness );
  edge1->SetColor( parois_color );
  AddComponent( edge1 );

  edge2 = new Cylinder( 500.0 + BORDER_THICKNESS, 500.0 + BORDER_THICKNESS, thickness );
  edge2->SetColor( rim_color );
  AddComponent( edge2 );

  border = new Disk( 500.0 + BORDER_THICKNESS, 500.0 );
  border->SetOffset( 0.0, 0.0, thickness );
  border->SetColor( rim_color );
  AddComponent( border );

  SetOffset( 0.0, room_height / 2.0, 0.0 );

}

void Hublot::SetAperture( double value ) {

  aperture = value;
  if ( aperture > room_width - 2 * BORDER_THICKNESS ) aperture = room_width - 2 * BORDER_THICKNESS;
  if ( aperture > room_height - 2 * BORDER_THICKNESS ) aperture = room_height - 2 * BORDER_THICKNESS;

  bulkhead->SetRadius( aperture / 2.0 );
  edge1->SetRadius( aperture / 2.0 );
  edge2->SetRadius( aperture / 2.0 + BORDER_THICKNESS );
  border->SetRadius( aperture / 2.0 + BORDER_THICKNESS, aperture / 2.0 );

}

/*******************************************************************************/

AfdEnvironment::AfdEnvironment( void ) {

  /*
   *  Viewing Distance Dial.
   */
  dial = new Dial();
  dial->SetPosition( 500.0, 1200.0, 2000.0 );
  AddComponent( dial );

  /*
   * Reference Distance
   */
  meter_stick = new MeterStick();
  meter_stick->SetPosition( -950.0, 1000.0, 1000.0 );
  AddComponent( meter_stick );
	bar_with_viewer = false;

  /*
   *  Eye height indicator.
   */
  indicator = new HeightIndicator();
  indicator->SetOffset( 0.0, 0.0, 200.0 );
  AddComponent( indicator );

}

AfdEnvironment::~AfdEnvironment( void ) {}

/*********************************************************************************/

// Set the viewpoint.

double dial_offset_x =     400.0;
double dial_offset_y =  -  300.0;
double dial_offset_z =  - 2000.0;

double stick_offset_x = - 950.0;
double stick_offset_y = - 600.0;
double stick_offset_z = - 4000.0;

double AfdEnvironment::SetViewingDistance ( double distance ) {
  if ( distance < 0.0 ) distance = 0.0;
  if ( distance > room_length ) distance = room_length;
  viewing_distance = distance;
  viewpoint->SetPosition( 0.0, viewing_height, viewing_distance );
  dial->SetPosition( dial_offset_x, viewing_height + dial_offset_y, viewing_distance + dial_offset_z );
  if ( bar_with_viewer ) meter_stick->SetPosition( stick_offset_x, viewing_height + stick_offset_y, viewing_distance + stick_offset_z );
	return( viewing_distance );
}

void AfdEnvironment::SetViewingHeight ( double height ) {
  viewing_height = height;
  viewpoint->SetPosition( 0.0, viewing_height, viewing_distance );
  dial->SetPosition( dial_offset_x, viewing_height + dial_offset_y, viewing_distance + dial_offset_z );
  if ( bar_with_viewer ) meter_stick->SetPosition( stick_offset_x, viewing_height + stick_offset_y, viewing_distance + stick_offset_z );
}


/*********************************************************************************/

void AfdEnvironment::SetCeilingHeight( double height ) {

  ceiling_height = height;
  ceiling->SetPosition( 0.0, height, room_length / 2.0 );

}

void AfdEnvironment::SetFloorHeight( double height ) {

  floor_height = height;
  deck->SetPosition( 0.0, height, room_length / 2.0 );

}

double AfdEnvironment::SetAperture( double opening ) {
  if ( opening < 0.0 ) opening = 0.0;
  if ( opening > room_width ) opening = room_width;
  double height_limit = room_height;
  if ( floor_height > 0 ) height_limit -= 2.0 * floor_height;
  if ( opening > height_limit - 2 * BORDER_THICKNESS ) opening = height_limit - 2 * BORDER_THICKNESS;
  aperture = opening;
  doorway->SetAperture( opening );
  return( aperture );
}

double AfdEnvironment::SetPerceivedEH( double height ) {
  if ( height < floor_height ) height = floor_height;
  if ( height > ceiling_height) height = ceiling_height;
  perceived_eh = height;
  indicator->SetPosition( 0.0, perceived_eh, 0.0 );
  return( perceived_eh );
}

double AfdEnvironment::SetPerceivedDistance( double distance ) {
  if ( distance < 0.0 ) distance = 0.0;
  if ( distance > dial->maximum ) distance = dial->maximum;
  perceived_distance = distance;
  dial->SetValue( perceived_distance );
  return( perceived_distance );
}

/*******************************************************************************/

AfdRoom::AfdRoom( ApertureType type, RoomTextures *tex, double width, double height, double length ) {


  // Object dimensions in the virtual world.
  // All values are in millimeters.

  room_width  =  width;
  room_height =  height; 
  room_length =  length;
  wall_thickness = 250.0;

  Slab *slab;

 /* 
  * Define a viewing projection with:
  *  45° vertical field-of-view - horizontal fov will be determined by window aspect ratio.
  *  60 mm inter-pupilary distance - the units don't matter to OpenGL, but all the dimensions
  *      that I give for the model room here are in mm.
  *  10.0 to 10000.0  depth clipping planes - making this smaller would improve the depth resolution.
  */
  viewpoint = new Viewpoint( 6.0, 45.0, 10.0, 20000.0);

  // Static objects in the room.

  SetColor( WHITE );
  SetPosition( 0.0, 0.0, 0.0 );

  // Right Wall
  right_wall = new Assembly;
  slab = new Slab( room_length, 1.5 * room_height, wall_thickness );
  if ( tex ) slab->texture = tex->right;
  right_wall->AddComponent( slab );
  AddComponent( right_wall );
  right_wall->SetOffset( 0.0, room_height / 2.0, 0.0 );
  right_wall->SetPosition( ( room_width + wall_thickness ) / 2.0, 0.0, room_length / 2.0 );
  right_wall->SetOrientation( -90.0, j_vector );

// Left Wall
  left_wall = new Assembly;
  slab = new Slab( room_length, 1.5 * room_height, wall_thickness );
  if ( tex ) slab->texture = tex->left;
  left_wall->AddComponent( slab );
  AddComponent( left_wall );
  left_wall->SetOffset( 0.0 , room_height / 2.0, 0.0 );
  left_wall->SetPosition( ( - room_width - wall_thickness ) / 2, 0.0, room_length / 2.0 );
  left_wall->SetOrientation( 90.0, j_vector );

  // Create a ceiling. 
  // Make it wide enough so as not to see the left and right edges.
  // The dimension Y sets the far edge in depth.
  ceiling = new Slab( 3.0 * room_width, room_length, wall_thickness );
  ceiling->SetOffset( 0.0,0.0, wall_thickness / 2 );
  ceiling->SetPosition( 0.0, room_height, room_length / 2.0 );
  ceiling->SetOrientation( 90.0, i_vector );
  ceiling->SetColor( WHITE );
  if ( tex ) ceiling->texture = tex->ceiling;
  AddComponent( ceiling );

  // Floor
  deck = new Slab( 3.0 * room_width, 2.0 * room_length, wall_thickness );
  deck->SetColor( WHITE );
  deck->SetOffset( 0.0, 0.0, wall_thickness / 2 );
  deck->SetPosition( 0.0, 0.0, 0.0 );
  deck->SetOrientation( -90.0, i_vector );
  if ( tex ) deck->texture = tex->floor;
  AddComponent( deck );

  // Background
  if ( tex ) {
    if ( tex->rear ) {
      backdrop = new Patch( 6.0 * room_width, 6.0 * room_height );
      backdrop->SetPosition( 0.0, 0.0, - room_length );
      backdrop->SetTexture( tex->front );
      AddComponent( backdrop );
    }
  }

  /*
   * Create the doorway.
   */

  switch ( type ) {

  case SLIDING:
    doorway = new SlidingDoors( room_width, room_height, wall_thickness );
    break;

  case HUBLOT:
    doorway = new Hublot( room_width, room_height, wall_thickness );
    break;

  }
  doorway->SetColor( bulkhead_color );
  AddComponent( doorway );


  SetAperture( 1000.0 );
  SetPerceivedEH( .75 * room_height );
  SetPerceivedDistance( 3000.0 );

}

/*******************************************************************************/

ISS::ISS( ApertureType type, RoomTextures *tex, double width, double height, double length ) {


  // Object dimensions in the virtual world.
  // All values are in millimeters.

  // The rack is slightly smaller than the full height of the module.
  // This leaves a border top and bottom.
  rack_height =  1600.0; // Real height of Columbus module walls (rack) in mm.
  rack_width =   1000.0; // Real width of rack in mm.

  // The Columbus module is 2 x 2 x 8 meters wide.
  room_width  =  width;
  room_height =  height; 
  room_length =  length;
  wall_thickness = 250.0;

  float blank_color[4] = { 0.15, 0.15, 0.05, 1.0 };

//  char *rack_texture_file = "epm.bmp";

  Slab *slab;

 /* 
  * Define a viewing projection with:
  *  45° vertical field-of-view - horizontal fov will be determined by window aspect ratio.
  *  60 mm inter-pupilary distance - the units don't matter to OpenGL, but all the dimensions
  *      that I give for the model room here are in mm.
  *  10.0 to 10000.0  depth clipping planes - making this smaller would improve the depth resolution.
  */
  viewpoint = new Viewpoint( 6.0, 45.0, 10.0, 20000.0);

  /*
   * The rack texture is 256 pixels wide by 512 high.
   * We map this onto a patch that is 2 meters wide by 4 meter high in the virtual scene.
   * It then gets pasted on the left and right walls. The pattern is repeated to fill the
   *  entire wall from left to right. 
   * A neutral panel is added above and below to avoid gaps if the floor or ceiling are
   *  above or below the limits of the panel.
   */
//  rack_texture = new Texture( rack_texture_file, rack_width, rack_height );


  // Static objects in the room.

  SetColor( WHITE );
  SetPosition( 0.0, 0.0, 0.0 );

  // Right Wall
  right_wall = new Assembly;
  slab = new Slab( room_length, rack_height, wall_thickness );
  if ( tex ) slab->texture = tex->right;
//  slab->texture = rack_texture;
  right_wall->AddComponent( slab );

  // Extend Above
  slab = new Slab( room_length, rack_height, wall_thickness );
  slab->SetPosition( 0.0, rack_height, 0.0 );
  slab->SetColor( blank_color );
  right_wall->AddComponent( slab );

  // Extend Below
  slab = new Slab( room_length, rack_height, wall_thickness );
  slab->SetPosition( 0.0, - rack_height, 0.0 );
  slab->SetColor( blank_color );
  right_wall->AddComponent( slab );

  AddComponent( right_wall );
  right_wall->SetOffset( 0.0, room_height / 2.0, 0.0 );
  right_wall->SetPosition( ( room_width + wall_thickness ) / 2.0, 0.0, room_length / 2.0 );
  right_wall->SetOrientation( -90.0, j_vector );

// Left Wall
  left_wall = new Assembly;
  slab = new Slab( room_length, rack_height, wall_thickness );
  if ( tex ) slab->texture = tex->left;
//  slab->texture = rack_texture;
  left_wall->AddComponent( slab );

  // Extend Above
  slab = new Slab( room_length, rack_height, wall_thickness );
  slab->SetPosition( 0.0, rack_height, 0.0 );
  slab->SetColor( blank_color );
  left_wall->AddComponent( slab );

  // Extend Below
  slab = new Slab( room_length, rack_height, wall_thickness );
  slab->SetPosition( 0.0, - rack_height, 0.0 );
  slab->SetColor( blank_color );
  left_wall->AddComponent( slab );

  AddComponent( left_wall );
  left_wall->SetOffset( 0.0 , room_height / 2.0, 0.0 );
  left_wall->SetPosition( ( - room_width - wall_thickness ) / 2, 0.0, room_length / 2.0 );
  left_wall->SetOrientation( 90.0, j_vector );

  // Create a ceiling. 
  // Make it wide enough so as not to see the left and right edges.
  // The dimension Y sets the far edge in depth.
  ceiling = new Slab( 6.0 * room_width, room_length, wall_thickness );
  ceiling->SetOffset( 0.0,0.0, wall_thickness / 2 );
  ceiling->SetPosition( room_width / 2.0, rack_height, 0.0 );
  ceiling->SetOrientation( 90.0, i_vector );
  ceiling->SetColor( WHITE );
  if ( tex ) ceiling->texture = tex->ceiling;
  AddComponent( ceiling );

  // Floor
//  deck = new Slab( 6.0 * room_width, 2.0 * room_length, wall_thickness );
//  deck = new Slab( 6.0 * room_width, 2.0 * room_length, wall_thickness );
  deck = new Slab( 6.0 * room_width, 1.1 * room_length, wall_thickness );
  deck->SetColor( WHITE );
  deck->SetOffset( 0.0,0.0, wall_thickness / 2 );
  deck->SetPosition( - room_width / 2.0, 0.0, 0.0 );
  deck->SetOrientation( -90.0, i_vector );
  if ( tex ) deck->texture = tex->floor;
  AddComponent( deck );

  // Background
  if ( tex ) {
    if ( tex->rear ) {
      backdrop = new Patch( 6.0 * room_width, 6.0 * room_height );
      backdrop->SetPosition( 0.0, 0.0, - room_length );
      backdrop->SetTexture( tex->front );
      AddComponent( backdrop );
    }
  }

  /*
   * Create the doorway.
   */

  switch ( type ) {

  case SLIDING:
    doorway = new SlidingDoors( room_width, room_height, wall_thickness );
    break;

  case HUBLOT:
    doorway = new Hublot( room_width, room_height, wall_thickness );
    break;

  }
  doorway->SetColor( bulkhead_color );
  AddComponent( doorway );


  SetAperture( 1000.0 );
  SetPerceivedEH( .75 * rack_height );
  SetPerceivedDistance( 3000.0 );

}


