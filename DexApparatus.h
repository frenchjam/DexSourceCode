/*********************************************************************************/
/*                                                                               */
/*                                DexApparatus.h                                 */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#ifndef DexApparatusH
#define DexApparatusH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>


#include "Dexterous.h"
#include "DexMonitor.h"
#include "DexTargets.h"
#include "DexSounds.h"
#include "DexTracker.h"

#include "VectorsMixin.h" 


/********************************************************************************/

//
// DEX Apparatus, including a tracker, a target bar and a means 
// to send messages to the ground.
//

class DexApparatus : public VectorsMixin {

	private:


	protected:

		unsigned long currentTargetState;

	public:

		// Keep track of what type we are.
		DexApparatusType  type;

		// An object that handles event logging and transmission to ground.
		DexMonitorServer		*monitor;
		// An object that handles the visible targets.
		DexTargets				*targets;
		// An object that handles the 3D motion tracking.
		DexTracker				*tracker;
		// An object that handles the sound generating hardware.
		DexSounds				*sounds;

		DexTimer	trialTimer;

		// The position of each of the visible targets in tracker coordinates.
		float	targetPosition[DEX_MAX_TARGETS][3];
		char	*targetPositionFilename;

		// Cache the latest marker data.
		CodaFrame	currentMarkerFrame;

		// To be more efficient, we cache the real-time position 
		// and orientation of the manipulandum.
		bool	manipulandumVisible;
		float	manipulandumPosition[3];
		float	manipulandumOrientation[4];

		// After an acquisition, the full data set is retrieved from
		// the tracker and analog system and stored here.
		CodaFrame			acquiredPosition[DEX_MAX_MARKER_FRAMES];			// Markers
		ManipulandumState	acquiredManipulandumState[DEX_MAX_MARKER_FRAMES];	// Manipulandum Position
		double				manipulandumVelocity[DEX_MAX_MARKER_FRAMES];		// Tangential velocity
		bool				velocityComputed;
		DexEvent			eventList[DEX_MAX_EVENTS];
		int					nEvents;

		Quit();

		int nMarkers;
		int nTones;
		int nTargets;
		int nVerticalTargets;
		int nHorizontalTargets;
		int nAcqFrames;
		int nCodas;

		DexApparatus( int n_vertical_targets = N_VERTICAL_TARGETS, 
							int n_horizontal_targets = N_HORIZONTAL_TARGETS,
							int tones = N_TONES, int n_markers = N_MARKERS );

		// Called inside loops to update current state.
		// This is strictly local to my simulator.
		virtual void Update( void );

		// Core routine to establish the position of each of the targets in 3D space.
		// This routine is to be called at the beginning of a trial to determine where the 
		// targets are in the current tracker reference frame.
		// Some apparattii will compute the positions from the current reference marker
		// positions while others will just load them from a file.
		virtual void SetTargetPositions( void );

		// These additional routines provide a generic method in which the manipulandum
		// is used to measure the position of each target and store them in a file.
		// The default method to set the target positions is to simply reload the file.
		virtual int CalibrateTargets( void );
		virtual void LoadTargetPositions( char *filename = "DexTargetPositions.csv" );

		// Core routines for stimulus control.
		virtual void SetTargetState( unsigned long target_state );
		virtual void SetSoundState( int tone, int volume );

		// Convenience routines for stimulus control. 
		virtual void TargetOn( int n );
		virtual void VerticalTargetOn( int n );
		virtual void HorizontalTargetOn( int n );
		virtual void TargetsOff( void );
		virtual int  DecodeTargetBits( unsigned long bits );

		virtual void SoundOn( int tone, int volume );
		virtual void SoundOff( void );
		virtual void Beep( int tone = BEEP_TONE, int volume = BEEP_VOLUME, float duration = BEEP_DURATION );
			
		// Acquisition
		virtual void StartAcquisition( float max_duration ); // TAKE OUT DURATION, add file tag and message
		virtual void StopAcquisition( void );
		virtual int  CheckOverrun(  const char *msg );	 // To be integrated with stop acquisition.
		virtual void SaveAcquisition( const char *tag ); // To be removed.
	
		// Flow control
		virtual void Wait( double seconds );
		virtual int  WaitSubjectReady( const char *message = "Ready to continue?" );
		virtual int  fWaitSubjectReady( const char *format = "Ready to continue?", ... );
		virtual int	 WaitUntilAtTarget( int target_id, const float desired_orientation[4], 
										float position_tolerance[3], float orientation_tolerance, 
										float hold_time, float timeout, char *msg  );

		virtual int  WaitUntilAtVerticalTarget( int target_id, const float desired_orientation[4] = uprightNullOrientation, float position_tolerance[3] = defaultPositionTolerance, float orientation_tolerance = defaultOrientationTolerance, float hold_time = waitHoldPeriod, float timeout = waitTimeLimit, char *msg = NULL );
		virtual int  WaitUntilAtHorizontalTarget( int target_id, const float desired_orientation[4] = uprightNullOrientation, float position_tolerance[3] = defaultPositionTolerance, float orientation_tolerance = defaultOrientationTolerance, float hold_time = waitHoldPeriod, float timeout = waitTimeLimit, char *msg = NULL );

		// Hardware configuration
		virtual int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );
		virtual DexSubjectPosture Posture( void );
		virtual DexTargetBarConfiguration BarPosition( void );
		virtual DexTappingSurfaceConfiguration TappingDeployment( void );	

		// Post hoc tests of data validity.
		virtual int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg = NULL );
		virtual int CheckMovementAmplitude(  float min, float max, float dirX, float dirY, float dirZ, const char *msg = NULL );
		virtual int CheckMovementAmplitude(  float min, float max, const Vector3 direction, char *msg = NULL );

		int CheckMovementCycles(  int min_cycles, int max_cycles, 
												float dirX, float dirY, float dirZ,
												float hysteresis, const char *msg );
		int CheckMovementCycles(  int min_cycles, int max_cycles, 
												const Vector3 direction,
												float hysteresis, const char *msg );
		int CheckEarlyStarts(  int n_false_starts, float hold_time, float threshold, float filter_constant, const char *msg = NULL );
		int CheckCorrectStartPosition( int target_id, float tolX, float tolY, float tolZ, int max_n_bad, const char *msg = NULL);
		int CheckMovementDirection(  int n_false_directions, float dirX, float dirY, float dirZ, float threshold, const char *msg = NULL );

		// Signalling events to the ground.
		virtual void SignalConfiguration( void );
		virtual void SignalEvent( const char *msg );

		// Mark events locally for post hoc analyses
		virtual void ClearEventLog( void );
		virtual void MarkEvent( int event, unsigned long param = 0x00L );

		virtual void MarkTargetEvent( unsigned int bits );
		virtual void MarkSoundEvent( int tone, int volume );

		virtual int TimeToFrame( float elapsed_time );
		virtual void FindAnalysisEventRange( int &first, int &last );
		virtual void FindAnalysisFrameRange( int &first, int &last );
		
		// Tracker installation and alignment.
		virtual bool DexApparatus::CheckTrackerFieldOfView( int unit, unsigned long marker_mask, 
										   float min_x, float max_x,
										   float min_y, float max_y,
										   float min_z, float max_z, const char *msg = "Markers not centered in view." );		
		virtual int PerformTrackerAlignment( const char *message = "Error performing the tracker alignment." );
		virtual int CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, const char *msg ="Codas out of alignment!" );
		virtual	int CheckTrackerPlacement( int unit, 
											const Vector3 expected_pos, float p_tolerance,
											const Quaternion expected_ori, float o_tolerance,
											const char *msg = "Codas bars not placed as expected.");

		// Measure the position and orientation of the manipulandum 
		// based on a frame of Coda marker data.
		virtual bool ComputeManipulandumPosition( float *pos, float *ori, CodaFrame &marker_frame );
		virtual bool ComputeTargetFramePosition( float *pos, float *ori, CodaFrame &marker_frame );

		// Get the latest marker data and compute from it the manipulandum position and orientation.
		virtual bool GetManipulandumPosition( Vector3 pos, Quaternion ori );
		// Knowing where the target frame is positioned might also be important.
//		virtual bool GetFramePosition( Vector3 pos, Quaternion ori );
		
		// Communicate to the subject that an error has occured and see what he wants to do.
		virtual int SignalError( unsigned int mb_type, const char *message = "Error." );
		virtual int fSignalError( unsigned int mb_type, const char *format, ... );

		virtual int SignalNormalCompletion( const char *message = "Task completed normally." );
		virtual int fSignalNormalCompletion( const char *format = "Task completed normally.", ... );

};

/************************************************************************************/

class DexVirtualApparatus : public DexApparatus {

private:

protected:

public:

	DexApparatusType	type;
	DexVirtualApparatus( int n_vertical_targets = N_VERTICAL_TARGETS, 
						 int n_horizontal_targets = N_HORIZONTAL_TARGETS,
						 int tones = N_TONES, int n_markers = N_MARKERS, int codas = N_CODAS );

};

/************************************************************************************/

class DexCodaApparatus : public DexApparatus {

private:

protected:

public:

	DexCodaApparatus( int n_vertical_targets = N_VERTICAL_TARGETS, 
					  int n_horizontal_targets = N_HORIZONTAL_TARGETS,
					  int tones = N_TONES, int n_markers = N_MARKERS );

};
/************************************************************************************/

class DexRTnetApparatus : public DexApparatus {

private:

protected:

public:

	DexRTnetApparatus( int n_vertical_targets = N_VERTICAL_TARGETS, 
					  int n_horizontal_targets = N_HORIZONTAL_TARGETS,
					  int tones = N_TONES, int n_markers = N_MARKERS );

};

/************************************************************************************/

class DexMouseApparatus : public DexApparatus {

private:

	// A dialog that allows one to set the apparatus configuration.
	HWND	dlg;

protected:

public:

	DexMouseApparatus( HINSTANCE hInstance, 
		int n_vertical_targets = N_VERTICAL_TARGETS, 
		int n_horizontal_targets = N_HORIZONTAL_TARGETS,
		int tones = N_TONES, int n_markers = N_MARKERS );
	DexSubjectPosture Posture( void );
	DexTargetBarConfiguration BarPosition( void );
	DexTappingSurfaceConfiguration TappingDeployment( void );	

};

/************************************************************************************/

class DexCompiler : public DexApparatus {

private:

	char *script_filename;
	FILE *fp;

	void CloseScript( void );

protected:

public:

	DexCompiler( int n_vertical_targets = N_VERTICAL_TARGETS, 
				 int n_horizontal_targets = N_HORIZONTAL_TARGETS,
				 int n_tones = N_TONES, int n_markers = N_MARKERS, int n_codas = N_CODAS, 
				 char *filename = DEFAULT_SCRIPT_FILENAME );

	void SetTargetState( unsigned long target_state );
	void SetSoundState( int tone, int volume );

	void StartAcquisition( void );
	void StopAcquisition( void );
	void SaveAcquisition( void );

	void Wait( double seconds );
	int  WaitSubjectReady( const char *message = "Ready to continue?" );
	int	 WaitUntilAtTarget( int target_id, float tolerance[3], float hold_time, float timeout, char *msg  );

	int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );

	int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time );

};

#endif