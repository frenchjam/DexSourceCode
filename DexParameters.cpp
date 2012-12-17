/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <useful.h>
#include <screen.h>
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
#include <VectorsMixin.h>

#include "Dexterous.h"
#include "DexMonitorServer.h"
#include "DexTargets.h"
#include "DexTracker.h"
#include "DexSounds.h"
#include "DexApparatus.h"

// Parameters used when waiting for the hand to be at a target.
float defaultPositionTolerance[3] = { 100.0, 25.0, 25.0 };	// Zone considered to be at the target.
float defaultOrientationTolerance = 60.0;					// Measured in degrees.
// The tolerance on the orientation above is set very high for the simulator,
// because the simulator intentionally makes the manipulandum rotate as you move toward the higher targets.

float waitBlinkPeriod = 0.2;		// LED blink rate when out of zone.
float waitHoldPeriod = 1.0;			// Required hold time in zone.
float waitTimeLimit = 10.0;			// Signal time out if we wait this long.

char *TargetBarString[] = { "Indifferent", "Left", "Right", "Unknown" };
char *PostureString[] = { "Indifferent", "Seated", "Supine", "Unknown" };
char *TappingSurfaceString[] = { "Indifferent", "Extended", "Folded", "Unknown" };

// Null orientation of the manipulandum when in the upright (seated) posture.
// Orientations are expressed as quaternions.
const float uprightNullOrientation[4] = { 0.0, 0.0, 0.0, 1.0 };
// Null orientation of the manipulandum when in the supine position.
// TO DO: This is wrong. Need to compute the required quaternion.
const float supineNullOrientation[4] = { (ROOT2/2.0), 0.0, 0.0, (ROOT2/2.0) }; 


// Position of the markers relative to the control point of the manipulandum.
// This is used both by DexApparatus to compute the position and orientation of the
// manipulandum from acquired marker positions, and by DexMouseTracker to simulate
// where each marker is according to the position of the mouse.

float		ManipulandumBody[MANIPULANDUM_MARKERS][3] = 
{
	{1.0, 0.0, 0.0}, 
	{0.0, 1.0, 0.0},  
	{0.0, 0.0, 1.0}, 
	{1.0, 1.0, 0.0}, 
	{1.0, 0.0, 1.0}, 
	{0.0, 1.0, 1.0}, 
	{1.0, 1.0, 1.0},  
	{2.0, 2.0, 2.0}
};
int nManipulandumMarkers = MANIPULANDUM_MARKERS;
int ManipulandumMarkerID[MANIPULANDUM_MARKERS] = {0, 1, 2, 3, 4, 5, 6, 7};

// The wrist markers also constitute a rigid body. 
// We can reconstruct the position and orientation of the wrist in the same
// way as for the manipulandum, but this functionality is not at present
// needed in the real-time DEX system.

float		WristBody[WRIST_MARKERS][3] = 
{
	{1.0, 0.0, 0.0}, 
	{0.0, 1.0, 0.0},  
	{0.0, 0.0, 1.0}, 
	{1.0, 1.0, 0.0}, 
	{1.0, 0.0, 1.0}, 
	{0.0, 1.0, 1.0}, 
	{1.0, 1.0, 1.0},  
	{2.0, 2.0, 2.0}
};
int nWristMarkers = WRIST_MARKERS;
int WristMarkerID[WRIST_MARKERS] = {8, 9, 10, 11, 12, 13, 14, 15};

// The target frame is not quite a rigig body, because the vertical bar 
// can move with respect to the horizontal box. But it is convenient to 
// define the marker positions in the same format for DexMouseTracker.

float		TargetFrameBody[TARGET_FRAME_MARKERS][3] =
{
	// This value must be the origin.
	{  0.0,   0.0,   0.0}, 
	// This marker defines the X axis, so it's X value should be the 
	//  distance from the origin marker, and Y and Z should be zero.
	{300.0,   0.0,   0.0},  
	// The following two vectors say where we expect to find the marker
	// at the top and bottom of the target bar when installed in the 
	// right position.
	// These values are set to work with the CodaMouseTracker simulator.
	{  0.0,   0.0,   -118.0},
	{  0.0, 230.0,   -118.0}

};
int nFrameMarkers = TARGET_FRAME_MARKERS;
int FrameMarkerID[TARGET_FRAME_MARKERS] = {16, 17, 18, 19};

// DexMouseTracker will return these values when the current
// Coda transformation is requested.

float		SimulatedCodaOffset[2][3] = {
	{  1000,    0.0, -2500.0 }, 
	{   0.0, -900.0, -2500.0 }
};

float	SimulatedCodaRotation[2][3][3] = {
	{{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}},
	{{1.0, 0.0, 0.0},{ 0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}}
};

/***************************************************************************/


char *ATICalFilename[2] = {

	"e:\\ATI Calibrations\\FT8884.cal",
	"e:\\ATI Calibrations\\FT8885.cal"

};

