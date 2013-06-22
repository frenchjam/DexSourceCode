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