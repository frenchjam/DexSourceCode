/*********************************************************************************/
/*                                                                               */
/*                                   DexTasks.h                                  */
/*                                                                               */
/*********************************************************************************/

/*
 * Routines that run specific tasks using the DEX simulator.
 */

#pragma once
	
// Possible apparatii that DexSimulatorApp will put together.
typedef enum { MOUSE_TRACKER, CODA_TRACKER, RTNET_TRACKER } TrackerType;
typedef enum { MOUSE_ADC, GLM_ADC } AdcType;
typedef enum { SCREEN_TARGETS, GLM_TARGETS } TargetType;
typedef enum { SCREEN_SOUNDS, GLM_SOUNDS, SOUNDBLASTER_SOUNDS } SoundType;

#define HORIZONTAL	0
#define VERTICAL	1

#define LEFT	0
#define RIGHT	1

#define UPRIGHT	0
#define SUPINE	1

#define OPEN	0
#define CLOSED	1

#define DEFAULT -1

#define MAX_SEQUENCE_ENTRIES	1024

// These are the user-defined markers. 
enum { FORCE_OK = 0, SLIP };

// Possible protocols.
enum { OFFSETS_TASK, TARGETED_TASK, DISCRETE_TASK, OSCILLATION_TASK, COLLISION_TASK, FRICTION_TASK, RUN_SCRIPT, CALIBRATE_TARGETS, INSTALL_PROCEDURE };

// Common parameters.
extern double maxTrialDuration;				// The maximum time for a single recording.

extern double baselineTime;					// Duration of pause at first target before starting movement.
extern double initialMovementTime;			// Time allowed to reach the starting position.

extern double continuousDropoutTimeLimit;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
extern double cumulativeDropoutTimeLimit;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
extern double beepDuration;					// How long to play a tone for a beep.
extern double flashDuration;				// How long to leave a target on for a flash.

extern unsigned int refMarkerMask;
extern double copTolerance;					// Tolerance on how well the fingers are centered on the manipulandum.
extern double copForceThreshold;			// Threshold of grip force to test if the manipulandum is in the hand.
extern double copWaitTime;					// Gives time to achieve the centered grip. 
											// If it is short (eg 1s) it acts like a test of whether a centered grip is already achieved.

extern unsigned long alignmentMarkerMask;	// A bit mask describing which markers are used to perform the alignment check.
extern unsigned long fovMarkerMask;			// A bit mask describing which markers are used to check the fov of each CODA.

// Some helper functions provided by DexSimulatorApp.
void BlinkAll ( DexApparatus *apparatus );
int RunScript( DexApparatus *apparatus, const char *filename );
int RunTargetCalibration( DexApparatus *apparatus, const char *params = NULL );
int ParseForPosture( const char *cmd );
int ParseForEyeState( const char *cmd );
int ParseForDirection ( DexApparatus *apparatus, const char *cmd, int &posture, int &bar_position, Vector3 &direction_vector, Quaternion &desired_orientation );
char *ParseForTargetFile ( const char *cmd );
char *ParseForDelayFile ( const char *cmd );
int LoadSequence( const char *filename, int   *sequence, const int max_entries );
int LoadSequence( const char *filename, float *sequence, const int max_entries );
DexMass ParseForMass ( const char *cmd );

// Here are the different tasks.
int RunTargeted( DexApparatus *apparatus, const char *params = NULL );
int RunOscillations( DexApparatus *apparatus, const char *params = NULL );
int RunCollisions( DexApparatus *apparatus, const char *params = NULL );
int RunDiscrete( DexApparatus *apparatus, const char *params = NULL );

int RunFrictionMeasurement( DexApparatus *apparatus, const char *params = NULL );
int RunTransducerOffsetCompensation( DexApparatus *apparatus, const char *params = NULL );
int RunInstall( DexApparatus *apparatus, const char *params = NULL );
