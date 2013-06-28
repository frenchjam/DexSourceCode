/***************************************************************************/
/*                                                                         */
/*                               DexADC.cpp                                */
/*                                                                         */
/***************************************************************************/

// Functionality common to all DexADC objects.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <fMessageBox.h>

#include <VectorsMixin.h>
#include <DexTimers.h>
#include <Dexterous.h>
#include "DexADC.h"

double DexADC::GetSamplePeriod( void ) { 
	return( samplePeriod ); 
}

void DexADC::CopyAnalogSample( AnalogSample &destination, AnalogSample &source ) {

	destination.time = source.time;
	for ( int chan = 0; chan < nChannels; chan++ ) destination.channel[chan] = source.channel[chan];

}

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
void DexADC::AllowPollingDuringAcquisition( void ) { allow_polling = true; };
