/*********************************************************************************/
/*                                                                               */
/*                                  Dexterous.h                                  */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#ifndef DexterousH
#define DexterousH

#include "DexUDPServices.h"

#define NORMAL_EXIT 0
#define ESCAPE_EXIT 1
#define IGNORE_EXIT 2
#define ERROR_EXIT 3
#define ABORT_EXIT 4
#define RETRY_EXIT 5

#define BLINK_PERIOD 0.166
#define N_ERROR_BLINKS 3
#define N_NORMAL_BLINKS 3

#define DEX_MAX_CODAS 8
#define DEX_MAX_MARKERS 28
#define DEX_MAX_TARGETS 32
#define DEX_MAX_DATA_FRAMES 20000

#define N_MARKERS 28
#define N_VERTICAL_TARGETS 13
#define N_HORIZONTAL_TARGETS 10
#define N_TARGETS (N_VERTICAL_TARGETS+N_HORIZONTAL_TARGETS)
#define N_TONES	8
#define N_VOLUMES 8
#define N_DATA_FRAMES 3000
#define N_CODAS 2

#define BEEP_TONE	4
#define BEEP_VOLUME	8
#define BEEP_DURATION 0.200

#define INVISIBLE -999.999

#define HORIZONTAL	0
#define VERTICAL	1


/* 
 * Allows one to select different configurations of the target LEDs.
 * LeftHand and RightHand refer to the hand used by the subject, not necessarily
 *  to the side of the box where the target array will be.
 */

#define DONT_CARE	-1

typedef enum { 
	TargetBarLeft = 0, 
	TargetBarRight, 
	TargetBarUnknown,
	MAX_TARGET_CONFIGURATIONS  
} DexTargetBarConfiguration;
extern char *TargetBarString[];

typedef enum { 
	PostureSeated, 
	PostureSupine,
	PostureUnknown
} DexSubjectPosture;
extern	char *PostureString[];

typedef enum {
	TappingExtended, 
	TappingFolded,
	TappingUnknown
} DexTappingSurfaceConfiguration;
extern	char *TappingSurfaceString[];

typedef struct {
	
	double position[3];
	double orientation[3];
	double targetPos[DEX_MAX_TARGETS][3];

} TargetArrayConfiguration;
extern char *TargetBarString[];

extern TargetArrayConfiguration targetArrayConfiguration[MAX_TARGET_CONFIGURATIONS];

// Parameters used when waiting for the hand to be at a target.

extern float defaultPositionTolerance[3];	// Zone considered to be at the target.
extern float defaultOrientationTolerance;

extern float waitBlinkPeriod;		// LED blink rate when out of zone.
extern float waitHoldPeriod;		// Required hold time in zone.
extern float waitTimeLimit;			// Signal time out if we wait this long.

extern const float iVector[3], jVector[3], kVector[3];
extern const float uprightNullOrientation[4], supineNullOrientation[4]; 

// Possible protocols.

enum { TARGETED_PROTOCOL, OSCILLATION_PROTOCOL, COLLISION_PROTOCOL, RUN_SCRIPT, CALIBRATE_TARGETS };

// Possible apparatii.

typedef enum { DEX_GENERIC_APPARATUS, DEX_VIRTUAL_APPARATUS, DEX_MOUSE_APPARATUS, DEX_CODA_APPARATUS, DEX_COMPILER } DexApparatusType;

#define DEFAULT_SCRIPT_FILENAME	"DexSampleScript.dex"


/********************************************************************************/

typedef struct {

	float	position[3];
	bool	visibility;

} CodaMarker;

typedef struct {

	CodaMarker	marker[DEX_MAX_MARKERS];
	float		time;

} CodaFrame;

typedef struct {

	float	position[3];
	float	orientation[4];
	bool	visibility;
	float	time;

} ManipulandumState;

#define CODA_MANIPULANDUM_MARKER 0
#define CODA_FRAME_MARKER 5


#endif