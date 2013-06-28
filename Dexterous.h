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

// Some useful constants.

#define NORMAL_EXIT 0
#define ESCAPE_EXIT 1
#define IGNORE_EXIT 2
#define ERROR_EXIT 3
#define ABORT_EXIT 4
#define RETRY_EXIT 5

#define INVISIBLE -999.999f
#define TARGETS_OFF -1

#define HORIZONTAL	0
#define VERTICAL	1

#define LEFT	0
#define RIGHT	1

#define DONT_CARE	0

// These are maximum values, used to allocate arrays.

#define DEX_MAX_CODAS 8
#define DEX_MAX_MARKERS 28
#define DEX_MAX_TARGETS 32
#define DEX_MAX_MARKER_FRAMES 20000
#define DEX_MAX_EVENTS 20000

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

extern float defaultPositionTolerance[3];	// Zone considered to be at the target.
extern float defaultOrientationTolerance;

extern float waitBlinkPeriod;		// LED blink rate when out of zone.
extern float waitHoldPeriod;		// Required hold time in zone.
extern float waitTimeLimit;			// Signal time out if we wait this long.

extern const float uprightNullOrientation[4], supineNullOrientation[4]; 

/********************************************************************************/

// One might ask why I don't use the VectorsMixin module and use the types
// Vector3 and Quaternion. There is an incompatibilty between the 3dMatrix library
// used for graphical objects and the new VectorsMixin module. I use arrays of floats
// here so that Dexterous.h can be inserted into either system.


typedef struct {

	float	position[3];
	bool	visibility;

} CodaMarker;

typedef struct {

	CodaMarker	marker[N_MARKERS];
	float		time;

} CodaFrame;

typedef struct {

	float	position[3];
	float	orientation[4];
	bool	visibility;
	float	time;

} ManipulandumState;

typedef struct {
	float time;
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

typedef struct {

	float			time;
	int				event;
	unsigned long	param;

} DexEvent;

// Rigid body models.
// These are used both by DexApparatus to compute the position and orientation of the
// manipulandum from acquired marker positions, and by DexMouseTracker to simulate
// where each marker is according to the position of the mouse. Because they are shared,
// I have made them global, rather than incorporating the model into Apparatus.


#define			MANIPULANDUM_MARKERS 8
extern float	ManipulandumBody[MANIPULANDUM_MARKERS][3];
extern int		nManipulandumMarkers;
extern int		ManipulandumMarkerID[MANIPULANDUM_MARKERS];

#define			WRIST_MARKERS 8
extern float	WristBody[WRIST_MARKERS][3];
extern int		nWristMarkers;
extern int		WristMarkerID[WRIST_MARKERS];

#define			TARGET_FRAME_MARKERS	4
extern float	TargetFrameBody[TARGET_FRAME_MARKERS][3];
extern int		nFrameMarkers;
extern int		FrameMarkerID[TARGET_FRAME_MARKERS];

// Identify the 4 reference markers.
// I am going to avoid the terms 'left', 'right', 'top' and 'bottom'
//  because this leads to confusion between the subject left and right
//  or left and right from the viewpoint of the CODAs.
#define DEX_NEGATIVE_BOX_MARKER	16
#define DEX_POSITIVE_BOX_MARKER	17
#define DEX_NEGATIVE_BAR_MARKER	18
#define DEX_POSITIVE_BAR_MARKER	19

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

// These are some constants that are used inside the protocol routines.
// I will probably move these to a separate file

// Minimum normal force to compute a center of pressure.
#define DEFAULT_COP_THRESHOLD	0.25	

// These are the user-defined markers. 
enum { FORCE_OK = 0, SLIP_OK };

// Possible protocols.
enum { TARGETED_PROTOCOL, OSCILLATION_PROTOCOL, COLLISION_PROTOCOL, FRICTION_PROTOCOL, RUN_SCRIPT, CALIBRATE_TARGETS, INSTALL_PROCEDURE };

// Possible apparatii.
typedef enum { DEX_NULL_APPARATUS, DEX_GENERIC_APPARATUS, DEX_DYNAMIC_APPARATUS, DEX_VIRTUAL_APPARATUS, DEX_MOUSE_APPARATUS, DEX_CODA_APPARATUS, DEX_RTNET_APPARATUS, DEX_COMPILER } DexApparatusType;

typedef enum { MOUSE_TRACKER, CODA_TRACKER, RTNET_TRACKER } TrackerType;
typedef enum { MOUSE_ADC, GLM_ADC } AdcType;
typedef enum { SCREEN_TARGETS, GLM_TARGETS } TargetType;
typedef enum { SCREEN_SOUNDS, SOUNDBLASTER_SOUNDS } SoundType;

