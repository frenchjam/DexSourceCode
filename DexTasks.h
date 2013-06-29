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
typedef enum { SCREEN_SOUNDS, SOUNDBLASTER_SOUNDS } SoundType;

// These are the user-defined markers. 
enum { FORCE_OK = 0, SLIP_OK };

// Possible protocols.
enum { TARGETED_PROTOCOL, OSCILLATION_PROTOCOL, COLLISION_PROTOCOL, FRICTION_PROTOCOL, RUN_SCRIPT, CALIBRATE_TARGETS, INSTALL_PROCEDURE };

// Common parameters.
extern double baselineTime;					// Duration of pause at first target before starting movement.
extern double continuousDropoutTimeLimit;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
extern double cumulativeDropoutTimeLimit;	// Duration in seconds of the maximum time for which the manipulandum can disappear.
extern double beepTime;						// How long to play a tone for a beep.
extern double flashTime;					// How long to leave a target on for a flash.

// Some helper functions provided by DexSimulatorApp.
void ShowStatus ( const char *message );
void HideStatus ( void );
void BlinkAll ( DexApparatus *apparatus );
int RunScript( DexApparatus *apparatus, const char *filename );
int RunTargetCalibration( DexApparatus *apparatus );

// Here are the different tasks.
int RunTargeted( DexApparatus *apparatus, DexTargetBarConfiguration bar_position, int target_sequence[], int n_targets  );
int RunOscillations( DexApparatus *apparatus );
int RunCollisions( DexApparatus *apparatus );

int RunFrictionMeasurement( DexApparatus *apparatus );
int RunTransducerOffsetCompensation( DexApparatus *apparatus );
int RunInstall( DexApparatus *apparatus );
