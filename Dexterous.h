/*********************************************************************************/
/*                                                                               */
/*                                  Dexterous.h                                  */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#pragma once

#include "DexUDPServices.h"
#include "VectorsMixin.h"

// Some useful constants.

#define NORMAL_EXIT 0
#define ESCAPE_EXIT 1
#define IGNORE_EXIT 2
#define ERROR_EXIT 3
#define ABORT_EXIT 4
#define RETRY_EXIT 5

#define INVISIBLE -999.999f
#define TARGETS_OFF -1

#define LEFT_ATI	0
#define RIGHT_ATI	1

#define DONT_CARE	0

// These are maximum values, used to allocate arrays.

#define DEX_MAX_CODAS 8
#define DEX_MAX_MARKERS 28
#define DEX_MAX_TARGETS 32
#define DEX_MAX_MARKER_FRAMES 20000
#define DEX_MAX_EVENTS 20000
#define DEX_MAX_TIMEOUT	300
#define DEX_MAX_DURATION 120.0

#define DEX_MAX_CHANNELS		64
#define DEX_MAX_ANALOG_SAMPLES	100000

// The next are nominal values describing the hardware.
// They are often used as the default for an optional parameter.

#define ANALOG_SAMPLE_PERIOD	0.001
#define MARKER_SAMPLE_PERIOD	0.005

#define N_MARKERS 28
#define N_VERTICAL_TARGETS 13
#define N_HORIZONTAL_TARGETS 10
#define N_TARGETS (N_VERTICAL_TARGETS+N_HORIZONTAL_TARGETS)
#define N_TONES	8
#define N_VOLUMES 8
#define N_DATA_FRAMES 3000
#define N_CODAS 2
#define N_CHANNELS	64
#define GLM_CHANNELS 16

#define N_FORCE_TRANSDUCERS		2
#define N_GAUGES				6	// Number of strain gauges per force/torque transducer.
#define N_SAMPLES_FOR_AVERAGE	20

#define LEFT_ATI_TRANSDUCER		0
#define RIGHT_ATI_TRANSDUCER	1

#define LEFT_ATI_FIRST_CHANNEL	0
#define RIGHT_ATI_FIRST_CHANNEL	6

// Here we have only one accelerometer.
#define HIGH_ACC_CHANNEL		12
#define LOW_ACC_FIRST_CHANNEL	12


// Behavior of some of the built-in routines.
// Can also be used in the paradigms.

#define BLINK_PERIOD 0.166
#define N_ERROR_BLINKS 3
#define N_NORMAL_BLINKS 3

#define BEEP_TONE	4
#define BEEP_VOLUME	8
#define BEEP_DURATION 0.200

/* 
 * The follwing constants allow one to select different configurations of the target LEDs.
 * LeftHand and RightHand refer to the hand used by the subject, not necessarily
 *  to the side of the box where the target array will be.
 */
typedef enum { 
	PostureIndifferent = 0,
	PostureSeated,
	PostureSupine,
	PostureUnknown
} DexSubjectPosture;
extern	char *PostureString[];

typedef enum { 
	TargetBarIndifferent = 0, 
	TargetBarLeft, 
	TargetBarRight, 
	TargetBarUnknown,
	MAX_TARGET_CONFIGURATIONS  
} DexTargetBarConfiguration;
extern char *TargetBarString[];

typedef enum {
	TappingIndifferent = 0, 
	TappingExtended, 
	TappingFolded,
	TappingUnknown
} DexTappingSurfaceConfiguration;
extern	char *TappingSurfaceString[];


// Parameters used when waiting for the hand to be at a target.

extern Vector3 defaultPositionTolerance;	// Zone considered to be at the target.
extern double defaultOrientationTolerance;

extern double waitBlinkPeriod;		// LED blink rate when out of zone.
extern double waitHoldPeriod;		// Required hold time in zone.
extern double waitTimeLimit;			// Signal time out if we wait this long.

extern const Quaternion uprightNullOrientation, supineNullOrientation; 

/********************************************************************************/

// Structures that hold the collected data.

typedef struct {

	Vector3	position;
	bool	visibility;

} CodaMarker;

typedef struct {

	CodaMarker	marker[N_MARKERS];
	double		time;

} CodaFrame;

typedef struct {

	Vector3		position;
	Quaternion	orientation;
	bool		visibility;
	double		time;

} ManipulandumState;

typedef struct {
	double time;
	float channel[N_CHANNELS]; 
} AnalogSample;

// Event codes that are kept in a local buffer with a time stamp. 
// Here we define some special ones used by DexApparatus
// to compute the post hoc tests on acquired data.
// Users may also signal other events in the script
// that may be useful for analizing the data on the ground.
// By convention, the system events will be negative and 
// users can define positive events in the scripts.

#define TARGET_EVENT		-1	// Will be raised whenever the targets change state.
#define SOUND_EVENT			-2	// Signals any change of the sound state.
#define BEGIN_ANALYSIS		-3	// These two signal where in a record analyses such as counting 
#define END_ANALYSIS		-4	//  the number of cycles in a movement should start and end.
#define ACQUISITION_START	-5	// Automatically raised when acquisition starts and ends.
#define ACQUISITION_STOP	-6
#define TRIGGER_MOVEMENT	-7	// Used to test for false starts.
#define TRIGGER_MOVE_UP		-8	// Used to test for tapping movements in the wrong direction.
#define TRIGGER_MOVE_DOWN	-9

#define ACQUISITION_SAVE	-666

typedef struct {

	double			time;
	int				event;
	unsigned long	param;

} DexEvent;

// Rigid body models.
// These are used both by DexApparatus to compute the position and orientation of the
// manipulandum from acquired marker positions, and by DexMouseTracker to simulate
// where each marker is according to the position of the mouse. Because they are shared,
// I have made them global, rather than incorporating the model into Apparatus.

#define GLM_MANIPULANDUM

#ifdef GLM_MANIPULANDUM
#define			MANIPULANDUM_MARKERS 4
#else
#define			MANIPULANDUM_MARKERS 8
#endif

extern Vector3	ManipulandumBody[MANIPULANDUM_MARKERS];
extern int		nManipulandumMarkers;
extern int		ManipulandumMarkerID[MANIPULANDUM_MARKERS];

#define			WRIST_MARKERS 8
extern Vector3	WristBody[WRIST_MARKERS];
extern int		nWristMarkers;
extern int		WristMarkerID[WRIST_MARKERS];

#define			TARGET_FRAME_MARKERS	4
extern Vector3	TargetFrameBody[TARGET_FRAME_MARKERS];
extern int		nFrameMarkers;
extern int		FrameMarkerID[TARGET_FRAME_MARKERS];

// Identify the 4 reference markers.
// I am going to avoid the terms 'left', 'right', 'top' and 'bottom'
//  because this leads to confusion between the subject left and right
//  or left and right from the viewpoint of the CODAs.
#define DEX_NEGATIVE_BOX_MARKER	22
#define DEX_POSITIVE_BOX_MARKER	23
#define DEX_NEGATIVE_BAR_MARKER	24
#define DEX_POSITIVE_BAR_MARKER	25

// A structure to hold the position and orientation of the target bar,
//  and the position of each target at the configuration.
typedef struct {
	float position[3];
	float orientation[4];
	double targetPos[DEX_MAX_TARGETS][3];
} TargetArrayConfiguration;
extern TargetArrayConfiguration targetArrayConfiguration[MAX_TARGET_CONFIGURATIONS];

// Paths to the files containing the ATI Force/Torque transducer calibrations.
// Each apparatus will have it's own calibration files and there will have to be a system
// to select the correct files at startup through a parameterization file or somthing like that.
// Be careful not to mix the two. It would have a disastrous effect on the 
// force and torque readings.
// These are the default values, using a random set of files that I happen to have on hand.

#define DEFAULT_LEFT_ATI_CALFILE	"..\\DexSourceCode\\FT7928.cal"
#define DEFAULT_RIGHT_ATI_CALFILE	"..\\DexSourceCode\\FT7927.cal"

// How much, in degrees, to rotate each ATI around it's own Z axis to 
// align the ATI reference frame with the manipulandum reference frame.
// Note that the right ATI coordinate frame will also be flipped 180°
//	to align with the left one. But that rotation is assumed to be exactly 180.
// To Do: The ATI documentation appears to say that this angle is 22.5 degrees, but 
// the GLMbox software appears to say 30. Better check.
#define LEFT_ATI_ROTATION	 22.5
#define RIGHT_ATI_ROTATION	 22.5
	

// The script interpreter will execute this file, if no other is specified.
#define DEFAULT_SCRIPT_FILENAME	"DexSampleScript.dex"

// Minimum normal force to compute a center of pressure.
#define DEFAULT_COP_THRESHOLD	0.25	

