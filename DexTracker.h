/*********************************************************************************/
/*                                                                               */
/*                                  DexTracker.h                                 */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#ifndef DexTrackerH
#define DexTrackerH

#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include <codasys.h>
#include <CodaUtilities.h>
#include <DexTimers.h>


#include "Dexterous.h"
#include "DexMonitor.h"
#include "DexTargets.h"


/********************************************************************************/

class DexTracker {

	private:

	protected:

	public:

		int nMarkers;
		int nFrames;
		int nAcqFrames;

		double samplePeriod;

		DexTracker();

		virtual void Initialize( void );
		virtual int  Update( void );
		virtual void Quit( void );

		virtual void	StartAcquisition( float max_duration );
		virtual void	StopAcquisition( void );
		virtual int		RetrieveMarkerFrames( CodaFrame frames[], int max_frames );
		virtual bool	GetCurrentMarkerFrame( CodaFrame &frame );

		CodaFrame		currentMarkerFrame;
		CodaFrame		recordedMarkerFrames[DEX_MAX_DATA_FRAMES];

		virtual double	GetSamplePeriod( void );
		virtual int		GetNumberOfCodas( void );
		virtual bool	GetAcquisitionState( void );
		virtual bool	CheckOverrun( void );

};

/********************************************************************************/

class DexVirtualTracker : public DexTracker {

private:

	DexTimer	oscillate_timer;

	int			next_sample;
	bool		acquisition_on;
	
protected:

public:

	int simulated_movement;
	int nCodas;

	DexVirtualTracker( DexTargets *targets );

	void Initialize( void );
	void SetMovementType( int type );

	int  Update( void );
	bool GetManipulandumPosition( float *pos, float *ori );
	bool GetFramePosition( float *pos );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	int  RetrievePositions( float *destination );

	bool GetAcquisitionState( void );

};

/********************************************************************************/

class DexCodaTracker : public DexTracker {

private:

	/* 
	 * Real-time marker data
	 * Defines storage for one Coda frame.
	 */
	CODA_FRAME_DATA_STRUCT coda_data_frame;
	codaBYTE bInView[ CODA_MAX_MARKERS ];
	codaFLOAT fPosition[ CODA_MAX_MARKERS * 3 ];

	CODA_ACQ_DATA_MULTI_STRUCT coda_multi_acq_frame;
	codaBYTE bInViewMulti[ CODA_MAX_MARKERS * DEX_MAX_DATA_FRAMES];
	codaFLOAT fPositionMulti[ CODA_MAX_MARKERS * DEX_MAX_DATA_FRAMES * 3 ];


protected:

public:

	int nCodas;

	DexCodaTracker( int n_codas = 2, int n_markers = N_MARKERS, int n_frames = N_DATA_FRAMES );

	void Initialize( void );
	int  Update( void );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	void CheckAcquisitionOverrun( void );

	bool GetAcquisitionState( void );
	int  GetNumberOfCodas( void );

	int		RetrieveMarkerFrames( CodaFrame frames[], int max_frames );
	bool	GetCurrentMarkerFrame( CodaFrame &frame );

};

/********************************************************************************/

class DexMouseTracker : public DexTracker {

private:

	bool		acquisitionOn;
	bool		overrun;
	DexTimer	acquisitionTimer;
	
protected:

public:

	DexMouseTracker();

	void Initialize( void );

	int  Update( void );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	bool GetAcquisitionState( void );
	bool CheckOverrun( void );

	int	 RetrieveMarkerFrames( CodaFrame frames[], int max_frames );
	bool GetCurrentMarkerFrame( CodaFrame &frame );

};

#endif