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
#include "DexApparatus.h"
#include "Dexterous.h"

// Parameters used when waiting for the hand to be at a target.
double defaultTolerance[3] = { 100.0, 25.0, 25.0 };	// Zone considered to be at the target.
double waitBlinkPeriod = 0.2;						// LED blink rate when out of zone.
double waitHoldPeriod = 1.0;						// Required hold time in zone.
double waitTimeLimit = 10.0;						// Signal time out if we wait this long.

char *TargetBarString[] = { "Left", "Right" };
char *PostureString[] = { "Seated", "Supine" };
char *TappingSurfaceString[] = { "Extended", "Folded" };

/***************************************************************************/


