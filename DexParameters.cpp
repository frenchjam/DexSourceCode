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

float waitBlinkPeriod = 0.2;						// LED blink rate when out of zone.
float waitHoldPeriod = 1.0;						// Required hold time in zone.
float waitTimeLimit = 10.0;						// Signal time out if we wait this long.

char *TargetBarString[] = { "Left", "Right" };
char *PostureString[] = { "Seated", "Supine" };
char *TappingSurfaceString[] = { "Extended", "Folded" };

// Null orientation of the manipulandum when in the upright (seated) posture.
// Orientations are expressed as quaternions.
const float uprightNullOrientation[4] = { 0.0, 0.0, 0.0, 1.0 };
// Null orientation of the manipulandum when in the supine position.
// TO DO: This is wrong. Need to compute the required quaternion.
const float supineNullOrientation[4] = { ROOT2, 0.0, 0.0, ROOT2 }; 

#define MANIPULANDUM_MARKERS 8
Vector3		ManipulandumBody[MANIPULANDUM_MARKERS] = 
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




/***************************************************************************/


