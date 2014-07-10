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

#define OPEN	0
#define CLOSED	1

#define DEFAULT -1


// These are the user-defined markers. 
enum { FORCE_OK = 0, SLIP };

// Possible protocols.
enum { AUDIO_CHECK, MISC_INSTALL, SHOW_PICTURES, OFFSETS_TASK, TARGETED_TASK, DISCRETE_TASK, OSCILLATION_TASK, COLLISION_TASK, FRICTION_TASK, RUN_SCRIPT, RUN_PROTOCOL, RUN_SESSION, RUN_SUBJECT, CALIBRATE_TARGETS, INSTALL_PROCEDURE };

// Common parameters.
extern double maxTrialDuration;				// The maximum time for a single recording.

extern double baselineDuration;				// Duration of pause at first target before starting movement.
extern double initialMovementDuration;		// Time allowed to reach the starting position.

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

extern int defaultCameraFrameRate;
 
extern char *OkToContinue;					// A standardized message about pressing OK to continue. Could be set to "" if we don't think those messages are needed.

// Some helper functions provided by DexSimulatorApp.

void AnalysisProgress( DexApparatus *apparatus, int which, int total, const char *message );

void RestartDirectives( DexApparatus *apparatus );
void AddDirective( DexApparatus *apparatus, const char *directive, const char *picture = NULL );
void ShowDirectives( DexApparatus *apparatus );
void ReadyToGo( DexApparatus *apparatus );

void BlinkAll ( DexApparatus *apparatus );
void SignalEndOfRecording ( DexApparatus *apparatus );

int RunScript( DexApparatus *apparatus, const char *filename );
int RunProtocol ( DexApparatus *apparatus, char *filename );
int RunSession ( DexApparatus *apparatus, char *filename );
int RunSubject ( DexApparatus *apparatus, char *filename );
int RunTargetCalibration( DexApparatus *apparatus, const char *params = NULL );
int ParseForEyeState( const char *cmd );
int ParseForDirection ( DexApparatus *apparatus, const char *cmd );
double ParseForFrequency ( DexApparatus *apparatus, const char *cmd );
double ParseForDuration ( DexApparatus *apparatus, const char *cmd );
double ParseForPinchForce( DexApparatus *apparatus, const char *cmd );
double ParseForFilterConstant( DexApparatus *apparatus, const char *cmd );
char *ParseForString ( const char *cmd, const char *key );

char *ParseForTargetFile ( const char *cmd );
char *ParseForDelayFile ( const char *cmd );
char *ParseForRangeFile ( const char *cmd );
char *ParseForTag( const char *cmd );

bool ParseForNoCheck( const char *cmd );

#define MAX_SEQUENCE_ENTRIES	1024
#define N_SEQUENCES	8
#define N_SIZES 3

#define SMALL 0
#define MEDIUM 1	
#define LARGE 2

int LoadSequence( int    *sequence, const char *filename );
int LoadSequence( double *sequence, const char *filename );

#define LOWER	0
#define MIDDLE	1
#define UPPER	2
void LoadTargetRange( int limits[3], const char *filename );

#define UP		1
#define DOWN	-1

DexMass ParseForMass ( const char *cmd );
DexSubjectPosture ParseForPosture( const char *cmd );
bool ParseForPrep ( const char *cmd );

// Here are the different tasks.
int RunTargeted( DexApparatus *apparatus, const char *params = NULL );
int RunOscillations( DexApparatus *apparatus, const char *params = NULL );
int RunCollisions( DexApparatus *apparatus, const char *params = NULL );
int RunDiscrete( DexApparatus *apparatus, const char *params = NULL );

int RunFrictionMeasurement( DexApparatus *apparatus, const char *params = NULL );
int RunTransducerOffsetCompensation( DexApparatus *apparatus, const char *params = NULL );
int RunInstall( DexApparatus *apparatus, const char *params = NULL );
int CheckInstall( DexApparatus *apparatus, DexSubjectPosture desired_posture, DexTargetBarConfiguration bar );
int ShowPictures( DexApparatus *apparatus, const char *params );
int CheckAudio( DexApparatus *apparatus, const char *params );
int MiscInstall ( DexApparatus *apparatus, const char *params );

// Some common messages.

extern char *MsgReadyToStart;
extern char *MsgReadyToStartOpen;
extern char *MsgReadyToStartClosed;
extern char *MsgAcquiringBaseline;
extern char *MsgGripNotCentered;
extern char *MsgMoveToBlinkingTarget;
extern char *MsgCheckGripCentered;
extern char *MsgTooLongToReachTarget;
extern char *MsgTrialOver;
extern char *MsgQueryReadySupine;
extern char *MsgQueryReadySeated;
extern char *OkToContinue;
extern char *MsgReleaseAndOK;

extern char *PlaceTargetBarRightOpen;
extern char *PlaceTargetBarRightFolded;
extern char *PlaceTargetBarLeft;


extern char *InstructPickUpManipulandum;
