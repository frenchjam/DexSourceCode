/*********************************************************************************/
/*                                                                               */
/*                                    DexADC.h                                   */
/*                                                                               */
/*********************************************************************************/

/*
 * Interface to the DEX hardware.
 */

#pragma once

#include "Dexterous.h"

/********************************************************************************/

class DexADC : public VectorsMixin {

	private:

	protected:

	public:

		// Number of ADC channels to be acquired.
		int nChannels;
		int nAcqSamples;

		double samplePeriod;

		DexADC() : nChannels( 32 ), samplePeriod( 0.001 ) {} ;

		virtual void Initialize( void );
		virtual int  Update( void );
		virtual void Quit( void );

		virtual void	StartAcquisition( float max_duration );
		virtual void	StopAcquisition( void );
		virtual bool	CheckAcquisitionOverrun( void );

		virtual int		RetrieveAnalogSamples( AnalogSample samples[], int max_samples );
		virtual bool	GetCurrentAnalogSample( AnalogSample &sample );

		AnalogSample	recordedAnalogSamples[DEX_MAX_ANALOG_SAMPLES];

		virtual double	GetSamplePeriod( void );
		virtual bool	GetAcquisitionState( void );
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

	DexMouseADC( HWND dlg = NULL ) : acquisitionOn(false), overrun(false) {
		this->dlg = dlg;
	}

	void Initialize( void );
	void Quit( void );

	int  Update( void );
	void StartAcquisition( float max_duration );
	void StopAcquisition( void );
	bool GetAcquisitionState( void );
	bool CheckOverrun( void );

	int		RetrieveAnalogSamples( AnalogSample samples[], int max_samples );
	bool	GetCurrentAnalogSample( AnalogSample &sample );

};

