/*********************************************************************************/
/*                                                                               */
/*                                    DexADC.h                                   */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#pragma once

#include <NIDAQmx.h>
#include "Dexterous.h"

/********************************************************************************/

class DexADC : public VectorsMixin {

	private:

	protected:

		bool allow_polling;

	public:

		// Number of ADC channels to be acquired.
		int nChannels;
		int nAcqSamples;

		double samplePeriod;

		DexADC() : nChannels( 0 ), samplePeriod( ANALOG_SAMPLE_PERIOD ), allow_polling( false ) {} ;

		virtual void Initialize( void ) = 0;
		virtual int  Update( void ) = 0;
		virtual void Quit( void ) = 0;

		// Not all the ADC interfaces, such as the NiDaq, allow continuous acquisition
		//  and at the same time periodic polling of the most recent values.
		// Calling this routine changes to a work-around solution.
		// It performs time-series acquisitions by repeatedly polling the 
		// device as quickly as it can and timestamps the samples. When the caller
		// retrieves the samples, the polled data is interpolated to generate time
		// series at a constant period.
		// The DEX hardware will allow both polling and continuous acquisition, so 
		//  no need to use it there. No script line will be generated for this command.
		// NB : This must be called before each continuous acquisition that requires 
		//  ADC polling, as StopAcquisition turns this mode off.
		void AllowPollingDuringAcquisition( void );

		virtual void	StartAcquisition( float max_duration ) = 0;
		virtual void	StopAcquisition( void ) = 0;
		virtual bool	CheckAcquisitionOverrun( void ) = 0;

		virtual int		RetrieveAnalogSamples( AnalogSample samples[], int max_samples ) = 0;
		virtual bool	GetCurrentAnalogSample( AnalogSample &sample ) = 0;

		AnalogSample	recordedAnalogSamples[DEX_MAX_ANALOG_SAMPLES];

		virtual double	GetSamplePeriod( void );
		virtual bool	GetAcquisitionState( void ) = 0;
		void			CopyAnalogSample( AnalogSample &destination, AnalogSample &source );

};


/********************************************************************************/

class DexMouseADC : public DexADC {

private:

	bool		acquisitionOn;
	bool		overrun;
	DexTimer	acquisitionTimer;
	double		duration;
	int			nPolled;

	HWND		dlg;
	FILE		*fp;
	
protected:

public:

	DexMouseADC( HWND dlg = NULL, int channels = N_CHANNELS ) : acquisitionOn(false), overrun(false) {
		this->dlg = dlg;
		nChannels = channels;
	}

	void Initialize( void );
	void Quit( void );

	int  Update( void );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	bool GetAcquisitionState( void );
	bool CheckAcquisitionOverrun( void );

	int		RetrieveAnalogSamples( AnalogSample samples[], int max_samples );
	bool	GetCurrentAnalogSample( AnalogSample &sample );

};

/********************************************************************************/

class DexNiDaqADC : public DexADC {

private:

	bool		acquisitionOn;
	bool		overrun;
	DexTimer	acquisitionTimer;
	double		duration;
	int			nPolled;

	FILE		*fp;
	
protected:

	TaskHandle  taskHandle;
	TaskHandle  continuousTaskHandle;
	void		ReportNiDaqError ( void );
public:

	DexNiDaqADC( void ) : acquisitionOn(false), overrun(false), taskHandle(0) { 
		nChannels = GLM_CHANNELS; 
	}

	void Initialize( void );
	void Quit( void );

	int  Update( void );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	bool GetAcquisitionState( void );
	bool CheckAcquisitionOverrun( void );

	int		RetrieveAnalogSamples( AnalogSample samples[], int max_samples );
	bool	GetCurrentAnalogSample( AnalogSample &sample );

};
