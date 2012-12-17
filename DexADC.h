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

		virtual void Initialize( void ) = 0;
		virtual int  Update( void ) = 0;
		virtual void Quit( void ) = 0;

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

	DexMouseADC( HWND dlg = NULL ) : acquisitionOn(false), overrun(false) {
		this->dlg = dlg;
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

