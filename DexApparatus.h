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

#include <ATIDAQ\ftconfig.h>

#include <VectorsMixin.h>

#include <DexTimers.h>
#include <DexTargets.h>
#include <DexSounds.h>
#include <DexTracker.h>
#include <DexADC.h>
#include <DexMonitorServer.h>

/********************************************************************************/

//
// DEX Apparatus, including a tracker, a target bar and a means 
// to send messages to the ground.
//

class DexApparatus : public VectorsMixin {
	
private:
	
	char filename_tag[256];
	FILE *fp;

	// These are parameters used to map measured forces to visible
	// target LEDs to provide feedback of the desired and actual
	// forces to be attained.
	int			min_grip_led, max_grip_led;
	int			min_load_led, max_load_led, zero_load_led;
	double		grip_to_led, grip_offset;
	double		load_to_led, load_offset;

	// This is used in the UpdateForceToLED() method.
	// It could also be used elsewhere to harmonize blinking, but 
	// I'm not taking the time to do that.
	DexTimer	blink_timer;
	bool		blink;

	// Saves force values between calls, so that recursive filtering 
	// can be applied.
	double		filteredLoad;
	double		filteredGrip;

	void MapForceToLED( float min_grip, float max_grip, float min_load, float max_load );
	void UpdateForceToLED( float grip, float load );


protected:
	
	unsigned long currentTargetState;
	unsigned long horizontalTargetMask;
	unsigned long verticalTargetMask;
	
public:
	
	// Identify the 4 reference markers.
	// I am going to avoid the terms 'left', 'right', 'top' and 'bottom'
	//  because this leads to confusion between the subject left and right
	//  or left and right from the viewpoint of the CODAs.
	static const int	negativeBoxMarker;
	static const int	positiveBoxMarker;
	static const int	negativeBarMarker;
	static const int	positiveBarMarker;
	
	// Identify the analog channels associated with each force/torque sensor.
	// Each element points to the index of the first channel for each sensor.
	static const int	ftAnalogChannel[N_FORCE_TRANSDUCERS];

	// This is the single high-g acceleration channel.
	static const int	highAccAnalogChannel;
	// This is the first of 3 low-g accelerometer channels.
	static const int	lowAccAnalogChannel;
	
	// Structures needed to use the ATI Force/Torque transducer library.
	Calibration			*ftCalibration[N_FORCE_TRANSDUCERS];
	Quaternion			ftAlignmentQuaternion[N_FORCE_TRANSDUCERS];

	// Path to the files holding the ATI calibrations.
	char	*ATICalFilename[N_FORCE_TRANSDUCERS];
	// Defines the rotation of each ATI sensor around the local Z axis.
	double	ATIRotationAngle[N_FORCE_TRANSDUCERS];
		
	// An object that handles event logging and transmission to ground.
	DexMonitorServer		*monitor;
	// An object that handles the visible targets.
	DexTargets				*targets;
	// An object that handles the 3D motion tracking.
	DexTracker				*tracker;
	// An object that handles the sound generating hardware.
	DexSounds				*sounds;
	// An object that performs analog acquisition.
	DexADC					*adc;
	
	DexTimer	trialTimer;
	
	// The position of each of the visible targets in tracker coordinates.
	Vector3	targetPosition[DEX_MAX_TARGETS];
	char	*targetPositionFilename;
	
	// After an acquisition, the full data set is retrieved from
	// the tracker and analog system and stored here.

	// Marker frames are one from each CODA unit, plus the combined.
	CodaFrame			acquiredPosition[DEX_MAX_CODAS+1][DEX_MAX_MARKER_FRAMES];			
	// The manipulandum position and orientation are computed from the marker data.
	ManipulandumState	acquiredManipulandumState[DEX_MAX_MARKER_FRAMES];
	
	AnalogSample		acquiredAnalog[DEX_MAX_ANALOG_SAMPLES];
	Vector3				acquiredForce[N_FORCE_TRANSDUCERS][DEX_MAX_ANALOG_SAMPLES];
	Vector3				acquiredTorque[N_FORCE_TRANSDUCERS][DEX_MAX_ANALOG_SAMPLES];
	Vector3				acquiredCOP[N_FORCE_TRANSDUCERS][DEX_MAX_ANALOG_SAMPLES];
	double				acquiredGripForce[DEX_MAX_ANALOG_SAMPLES];
	double				acquiredLoadForceMagnitude[DEX_MAX_ANALOG_SAMPLES];
	Vector3				acquiredLoadForce[DEX_MAX_ANALOG_SAMPLES];
	Vector3				acquiredAcceleration[DEX_MAX_ANALOG_SAMPLES];
	double				acquiredHighAcceleration[DEX_MAX_ANALOG_SAMPLES];
	DexEvent			eventList[DEX_MAX_EVENTS];
	int					nEvents;
		
	int nCodas;
	int nMarkers;
	int nTones;
	int nTargets;
	int nVerticalTargets;
	int nHorizontalTargets;
	int nChannels;
	int nForceTransducers;
	int nGauges;
	int nSamplesForAverage;
	
	int nAcqFrames;
	int nAcqSamples;
	
	DexApparatus::DexApparatus( void );
	DexApparatus::DexApparatus(	DexTracker			*tracker,
								DexTargets			*targets,
								DexSounds			*sounds,
								DexADC				*adc 
							);
	
	// Called once all the components are defined.
	virtual void Initialize( void );
	
	// Called inside loops to update current state.
	// This is strictly local to my simulator.
	virtual void Update( void );

	// Undo whatever needs to be done once the apparatus has been initialized.
	virtual void Quit();

	
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
	virtual void SetTargetStateInternal( unsigned long target_state );
	virtual void SetSoundStateInternal( int tone, int volume );
	
	// Convenience routines for stimulus control. 
	virtual void SetTargetState( unsigned long target_state );
	virtual void SetSoundState( int tone, int volume );
	virtual void TargetOn( int n );
	virtual void TargetOff( int n );
	virtual void VerticalTargetOn( int n );
	virtual void VerticalTargetOff( int n );
	virtual void HorizontalTargetOn( int n );
	virtual void HorizontalTargetOff( int n );
	virtual void TargetsOff( void );
	virtual int  DecodeTargetBits( unsigned long bits );
	
	virtual void SoundOn( int tone, int volume );
	virtual void SoundOff( void );
	virtual void Beep( int tone = BEEP_TONE, int volume = BEEP_VOLUME, float duration = BEEP_DURATION );
	
	// Acquisition
	virtual void StartAcquisition( const char *tag, float max_duration = DEX_MAX_DURATION ); 
	virtual int  StopAcquisition( const char *msg = "Error - Maybe file overrun." );
	virtual int  CheckOverrun(  const char *msg );	 // To be integrated with stop acquisition.
	virtual void SaveAcquisition( const char *tag ); // To be integrated with stop acquisition.
	
	// Flow control
	virtual void Wait( double seconds );
	virtual int  WaitSubjectReady( const char *message = "Ready to continue?" );
	virtual int  fWaitSubjectReady( const char *format = "Ready to continue?", ... );
	virtual int	 WaitUntilAtTarget( int target_id,  
									const Quaternion desired_orientation = uprightNullOrientation, 
									Vector3 position_tolerance = defaultPositionTolerance, 
									double orientation_tolerance = defaultOrientationTolerance, 
									double hold_time = waitHoldPeriod, double timeout = waitTimeLimit, 
									char *msg = NULL );
	
	virtual int  WaitUntilAtVerticalTarget( int target_id, 
											const Quaternion desired_orientation = uprightNullOrientation, 
											Vector3 position_tolerance = defaultPositionTolerance, 
											double orientation_tolerance = defaultOrientationTolerance, 
											double hold_time = waitHoldPeriod, double timeout = waitTimeLimit, 
											char *msg = NULL );
	virtual int  WaitUntilAtHorizontalTarget( int target_id, 
											const Quaternion desired_orientation = uprightNullOrientation, 
											Vector3 position_tolerance = defaultPositionTolerance, 
											double orientation_tolerance = defaultOrientationTolerance, 
											double hold_time = waitHoldPeriod, double timeout = waitTimeLimit, 
											char *msg = NULL );

	virtual int	 WaitCenteredGrip( float tolerance, float min_force, float timeout, char *msg = NULL  );

	virtual int	 WaitDesiredForces( float min_grip, float max_grip, 
									 float min_load, float max_load,
									 Vector3 direction, float filter_constant,
									 float hold_time, float timeout, const char *msg = NULL );
	virtual int WaitSlip( float min_grip, float max_grip, 
									 float min_load, float max_load, 
									 Vector3 direction,
									 float filter_constant,
									 float slip_threshold, 
									 float timeout, const char *msg = NULL );

	// Hardware configuration
	virtual int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );
	virtual DexSubjectPosture Posture( void );
	virtual DexTargetBarConfiguration BarPosition( void );
	virtual DexTappingSurfaceConfiguration TappingDeployment( void );	
	
	// Post hoc tests of data validity.
	virtual int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg = NULL );
	virtual int CheckMovementAmplitude(  double min, double max, double dirX, double dirY, double dirZ, const char *msg = NULL );
	virtual int CheckMovementAmplitude(  double min, double max, const Vector3 direction, char *msg = NULL );
	
	virtual int CheckMovementCycles(  int min_cycles, int max_cycles, 
								float dirX, float dirY, float dirZ,
								float hysteresis, const char *msg );
	virtual int CheckMovementCycles(  int min_cycles, int max_cycles, 
								const Vector3 direction,
								float hysteresis, const char *msg );
	virtual int CheckEarlyStarts(  int n_false_starts, float hold_time, float threshold, float filter_constant, const char *msg = NULL );
	virtual int CheckCorrectStartPosition( int target_id, float tolX, float tolY, float tolZ, int max_n_bad, const char *msg = NULL);
	virtual int CheckMovementDirection(  int n_false_directions, float dirX, float dirY, float dirZ, float threshold, const char *msg = NULL );
	virtual int CheckMovementDirection(  int n_false_directions, Vector3 direction, float threshold, const char *msg = NULL );
	virtual int CheckForcePeaks( float min_force, float max_force, int max_bad_peaks, const char *msg = NULL );
	virtual int CheckAccelerationPeaks( float min_amplitude, float max_amplitude, int max_bad_peaks, const char *msg = NULL );
	
	// Signalling events to the ground.
	virtual void SignalConfiguration( void );
	virtual void SignalEvent( const char *msg );
	
	// Mark events locally for post hoc analyses
	virtual void ClearEventLog( void );
	virtual void MarkEvent( int event, unsigned long param = 0x00L );
	virtual void Comment( const char *txt );
	
	virtual void MarkTargetEvent( unsigned int bits );
	virtual void MarkSoundEvent( int tone, int volume );
	
	virtual int TimeToFrame( float elapsed_time );
	virtual int TimeToSample( float elapsed_time );
	virtual void FindAnalysisEventRange( int &first, int &last );
	virtual void FindAnalysisFrameRange( int &first, int &last );
	
	// Tracker installation and alignment.
	virtual int CheckTrackerFieldOfView( int unit, unsigned long marker_mask, 
											float min_x, float max_x,
											float min_y, float max_y,
											float min_z, float max_z, 
											const char *msg = "Markers not centered in view." );		
	virtual int PerformTrackerAlignment( const char *message = "Error performing the tracker alignment." );
	virtual int CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, 
										const char *msg ="Codas out of alignment!" );
	virtual	int CheckTrackerPlacement( int unit, 
										const Vector3 expected_pos, float p_tolerance,
										const Quaternion expected_ori, float o_tolerance,
										const char *msg = "Codas bars not placed as expected.");
	
	// Measure the position and orientation of the manipulandum 
	// based on a frame of Coda marker data.
	virtual bool ComputeManipulandumPosition( Vector3 pos, Quaternion ori, 
												CodaFrame &marker_frame, 
												Quaternion default_orientation = NULL );
	virtual bool ComputeTargetFramePosition( Vector3 pos, Quaternion ori, CodaFrame &marker_frame );
	
	// Get the latest marker data and compute from it the manipulandum position and orientation.
	virtual bool GetManipulandumPosition( Vector3 pos, Quaternion ori, Quaternion default_orientation = NULL );
	// Knowing where the target frame is positioned might also be important.
	virtual bool GetTargetFramePosition( Vector3 pos, Quaternion ori );
	
	// Communicate to the subject that an error has occured and see what he wants to do.
	virtual int SignalError( unsigned int mb_type, const char *message = "Error." );
	virtual int fSignalError( unsigned int mb_type, const char *format, ... );
	
	virtual int SignalNormalCompletion( const char *message = "Task completed normally." );
	virtual int fSignalNormalCompletion( const char *format = "Task completed normally.", ... );
	
	// Methods to deal with the manipulandum force/torque measurements.
	virtual void InitForceTransducers( void );
	virtual void ReleaseForceTransducers( void );
	virtual void ZeroForceTransducers( void );
	virtual void NullifyStrainGaugeOffsets( int unit, float gauge_offsets[N_GAUGES] );
	virtual void ComputeAndNullifyStrainGaugeOffsets( void );
	
	void ComputeForceTorque( Vector3 &force, Vector3 &torque, int unit, AnalogSample analog );
	void GetForceTorque( Vector3 &force, Vector3 &torque, int unit );
	double ComputeCOP( Vector3 &cop, Vector3 &force, Vector3 &torque, double threshold = DEFAULT_COP_THRESHOLD );
	double GetCOP( Vector3 &cop, int unit, double threshold = DEFAULT_COP_THRESHOLD );
	float ComputeGripForce( Vector3 &force1, Vector3 &force2 );
	float GetGripForce( void );
	
	float ComputeLoadForce( Vector3 &load, Vector3 &force1, Vector3 &force2 );
	float ComputePlanarLoadForce( Vector3 &load, Vector3 &force1, Vector3 &force2 );
	float GetLoadForce( Vector3 &load );
	float GetPlanarLoadForce( Vector3 &load );

	float FilteredLoad( float new_load, float filter_constant );
	float FilteredGrip( float new_grip, float filter_constant );

};

/************************************************************************************/

class DexCompiler : public DexApparatus {

private:

	char *script_filename;
	FILE *fp;
	unsigned long nextStep;

	void CloseScript( void );
	void UnhandledCommand( const char *cmd );

	// My simulator assumes that all targets are controlled with a 32-bit word.
	// DEX uses two separate 16-bit words, one for vertical and one for horizontal.
	// These routines will translate between the two.
	unsigned short verticalTargetBits( unsigned long targetBits );
	unsigned short horizontalTargetBits( unsigned long targetBits );
	unsigned short verticalTargetBit( int target_id );
	unsigned short horizontalTargetBit( int target_id);

	char *quoteMessage( const char *message );

protected:

public:

	DexCompiler( DexTracker			*tracker,
				 DexTargets			*targets,
				 DexSounds			*sounds,
				 DexADC				*adc,
				 char *filename = DEFAULT_SCRIPT_FILENAME );

	void Initialize( void );
	void Quit( void );

	void SetTargetStateInternal( unsigned long target_state );
	void SetSoundStateInternal( int tone, int volume );

	void StartAcquisition( const char *tag, float max_duration = DEX_MAX_DURATION );
	int  StopAcquisition( const char *msg = "Buffer overrun or error writing data file." );

	void Wait( double seconds );
	int  WaitSubjectReady( const char *message = "Ready to continue?" );
	int	 WaitUntilAtTarget( int target_id, 
									const Quaternion desired_orientation,
									Vector3 position_tolerance, 
									double orientation_tolerance,
									double hold_time, 
									double timeout, 
									char *msg = "Timeout waiting to reach target." );
	int	 WaitCenteredGrip( float tolerance, float min_force, float timeout, char *msg = "Grip not centered"  );
	int	 WaitDesiredForces( float min_grip, float max_grip, 
									 float min_load, float max_load,
									 Vector3 direction, float filter_constant,
									 float hold_time, float timeout, const char *msg = "Desired force not achieved." );
	int WaitSlip( float min_grip, float max_grip, 
									 float min_load, float max_load, 
									 Vector3 direction,
									 float filter_constant,
									 float slip_threshold, 
									 float timeout, const char *msg = "Slip not achieved." );

	int SelectAndCheckConfiguration( int posture, int bar_position, int tapping );

	int CheckVisibility( double max_cumulative_dropout_time, double max_continuous_dropout_time, const char *msg = "Manipulandum occluded too often." );
	int CheckOverrun(  const char *msg );
	int CheckMovementAmplitude(  double min, double max, 
								 double dirX, double dirY, double dirZ,
								 const char *msg = "Movement amplitude outside range." );
	int CheckMovementCycles(  int min_cycles, int max_cycles, 
								float dirX, float dirY, float dirZ,
								float hysteresis, const char *msg );
	int CheckEarlyStarts(  int n_false_starts, float hold_time, float threshold, float filter_constant, const char *msg = NULL );
	int CheckCorrectStartPosition( int target_id, float tolX, float tolY, float tolZ, int max_n_bad, const char *msg = NULL);
	int CheckMovementDirection(  int n_false_directions, float dirX, float dirY, float dirZ, float threshold, const char *msg = NULL );
	int CheckForcePeaks( float min_force, float max_force, int max_bad_peaks, const char *msg = NULL );
	int CheckAccelerationPeaks( float min_amplitude, float max_amplitude, int max_bad_peaks, const char *msg = NULL );
	void ComputeAndNullifyStrainGaugeOffsets( void );

	void MarkEvent( int event, unsigned long param = 0x00L );

	int PerformTrackerAlignment( const char *message = "Error performing the tracker alignment." );
	int CheckTrackerFieldOfView( int unit, unsigned long marker_mask, 
											float min_x, float max_x,
											float min_y, float max_y,
											float min_z, float max_z, 
											const char *msg = "Markers not centered in view." );		
	int CheckTrackerAlignment( unsigned long marker_mask, float tolerance, int n_good, 
										const char *msg ="Codas out of alignment!" );
	int CheckTrackerPlacement( int unit, 
										const Vector3 expected_pos, float p_tolerance,
										const Quaternion expected_ori, float o_tolerance,
										const char *msg = "Codas bars not placed as expected.");


	void SetTargetPositions( void );
	void SignalConfiguration( void );
	void SignalEvent( const char *event );
	void Comment( const char *txt );

	void AddStepNumber( void );

};

#endif