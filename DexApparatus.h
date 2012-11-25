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


/********************************************************************************/

//
// DEX Apparatus, including a tracker, a target bar and a means 
// to send messages to the ground.
//

class DexApparatus {

	private:

	protected:

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

		// The position of each of the visible targets in tracker coordinates.
		float	targetPosition[DEX_MAX_TARGETS][3];
		char	*targetPositionFilename;

		// To be more efficient, we cache the real-time position 
		// and orientation of the manipulandum.
		bool	manipulandumVisible;
		float	manipulandumPosition[3];
		float	manipulandumOrientation[4];

		// After an acquisition, the full data set is retrieved from
		// the tracker and analog system and stored here.
		CodaFrame			acquiredPosition[DEX_MAX_DATA_FRAMES];			// Markers
		ManipulandumState	acquiredManipulandumState[DEX_MAX_DATA_FRAMES];	// Manipulandum Position

		DexApparatus( int n_vertical_targets = N_VERTICAL_TARGETS, 
					  int n_horizontal_targets = N_HORIZONTAL_TARGETS,
					  int tones = N_TONES, int n_markers = N_MARKERS, int codas = N_CODAS );
		Quit();

		int nCodas;
		int nMarkers;
		int nTones;
		int nTargets;
		int nVerticalTargets;
		int nHorizontalTargets;
		int nAcqFrames;

		// Initialization routines.

		// Some routines need to know where the targets are.
		virtual int CalibrateTargets( void );
		virtual void LoadTargetPositions( char *filename = "DexTargetPositions.csv" );

		// Called inside loops to update current state.
		virtual void Update( void );

		// The following are the user-accessible methods.
		// The compiler and scripts must be able to handle these.

		// Subject Communication
		virtual int WaitSubjectReady( const char *message = "Ready to continue?" );

		// Stimulus Control
		virtual void SetTargetState( unsigned long target_state );
		virtual void SetSoundState( int tone, int volume );
			
		// Acquisition
		virtual void StartAcquisition( float max_duration ); // TAKE OUT DURATION, add file tag and message
		virtual void StopAcquisition( void );
		virtual int  CheckOverrun(  const char *msg );	 // To be integrated with stop acquisition.
		virtual void SaveAcquisition( const char *tag ); // To be removed.
	
		// Flow control
		virtual void Wait( double seconds );
		virtual int  WaitUntilAtTarget( int targetID, double *tolerance = defaultTolerance );

		// Hardware configuration
		virtual int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );

		// Post hoc tests of data validity.
		virtual int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg = NULL );
		virtual int CheckMovementAmplitude(  float min, float max, float dirX, float dirY, float dirZ, const char *msg = NULL );

		// Logging and signalling events to the ground.
		virtual void SignalConfiguration( void );
		virtual void SignalEvent( const char *msg );
		
		// The user-accessible methods require the following methods.

		// Detection of the hardware configuration.
		virtual DexSubjectPosture Posture( void );
		virtual DexTargetBarConfiguration BarPosition( void );
		virtual DexTappingSurfaceConfiguration TappingDeployment( void );	
		
		virtual bool ComputeManipulandumPosition( float *pos, float *ori, CodaFrame &marker_frame );

		// These are simply the means to access the cached real-time manipulandum data.
		virtual bool GetManipulandumPosition( float *pos, float *ori );
		virtual bool GetFramePosition( double *pos );
		
		// Communicate to the subject that an error has occured and see what he wants to do.
		virtual int SignalError( unsigned int mb_type, const char *message = "Error." );
		virtual int fSignalError( unsigned int mb_type, const char *format, ... );

		// The following are convenience methods that simplify the simulator programs.
		// They build on the above methods and so do not need to be implemented by the
		// DEX system.

		virtual int SignalNormalCompletion( const char *message = "Task completed normally." );
		virtual int fSignalNormalCompletion( const char *format = "Task completed normally.", ... );

		virtual void TargetOn( int n );
		virtual void VerticalTargetOn( int n );
		virtual void HorizontalTargetOn( int n );
		virtual void TargetsOff( void );

		virtual void SoundOn( int tone, int volume );
		virtual void SoundOff( void );
		virtual void Beep( int tone = BEEP_TONE, int volume = BEEP_VOLUME, float duration = BEEP_DURATION );

		virtual int fWaitSubjectReady( const char *format = "Ready to continue?", ... );

		virtual int  WaitUntilAtVerticalTarget( int target_id, double *tolerance = defaultTolerance );
		virtual int  WaitUntilAtHorizontalTarget( int target_id, double *tolerance = defaultTolerance );


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
					  int tones = N_TONES, int n_markers = N_MARKERS, int codas = N_CODAS );

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
		int tones = N_TONES, int n_markers = N_MARKERS, int codas = 2 );
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
	int  WaitUntilAtTarget( int targetID, double *tolerance = defaultTolerance );

	int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );

	int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time );

};

#endif