/***************************************************************************/
/*                                                                         */
/*                                DexMouseADC                              */
/*                                                                         */
/***************************************************************************/

// This is a simulated analog interface to the DEX hardware.
// It simulates voltage inputs.

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
#include "..\DexSimulatorApp\resource.h"


/*********************************************************************************/

void DexMouseADC::Initialize( void ) {
	acquisitionOn = false; 
	overrun = false;
	nAcqSamples = 0;
	nPolled = 0;

	char *filename = "DexMouseAnalogDebug.mrk";

	// Open a file to store the analog values that were used by 
	// GetCurrentAnalogSample() to simulate the analog inputs. 

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fMessageBox( MB_OK, "DexMouseAnalog", "Error openning file for write:\n %s", filename );
		exit( -1 );
	}
	fprintf( fp, "Time" );
	for ( int chan = 0; chan < nChannels; chan++ ) fprintf( fp, "\tCh%02d", chan );
	fprintf( fp, "\n" );

}

void DexMouseADC::Quit( void ) {
	fclose( fp );
}


/*********************************************************************************/

void DexMouseADC::StartAcquisition( float max_duration ) { 
	acquisitionOn = true; 
	overrun = false;
	nPolled = 0;
	DexTimerSet( acquisitionTimer, max_duration );
	Update();
}

void DexMouseADC::StopAcquisition( void ) {
	acquisitionOn = false;
	duration = DexTimerElapsedTime( acquisitionTimer );
}

bool DexMouseADC::GetAcquisitionState( void ) {
	return( acquisitionOn );
}

bool DexMouseADC::CheckOverrun( void ) {
	return( overrun );
}

/*********************************************************************************/

int DexMouseADC::RetrieveAnalogSamples( AnalogSample samples[], int max_samples ) {
	
	int previous;
	int next;
	int smpl;
	
	double	time, interval, offset, relative;
	
	// Copy data into an array.
	previous = 0;
	next = 1;
	
	// Fill an array of frames at a constant frequency by interpolating the
	// frames that were taken at a variable frequency in real time.
	for ( smpl = 0; smpl < max_samples; smpl++ ) {
		
		// Compute the time of each slice at a constant sampling frequency.
		time = (double) smpl * samplePeriod;
		samples[smpl].time = (float) time;
		
		// Fill the first frames with the same position as the first polled frame;
		if ( time < recordedAnalogSamples[previous].time ) {
			CopyAnalogSample( samples[smpl], recordedAnalogSamples[previous] );
		}
		
		else {
			// See if we have caught up with the real-time data.
			if ( time > recordedAnalogSamples[next].time ) {
				previous = next;
				// Find the next real-time frame that has a time stamp later than this one.
				while ( recordedAnalogSamples[next].time <= time && next < nPolled ) next++;
				// If we reached the end of the real-time samples, then we are done.
				if ( next >= nPolled ) {
					break;
				}
			}
			
			// Compute the time difference between the two adjacent real-time frames.
			interval = recordedAnalogSamples[next].time - recordedAnalogSamples[previous].time;
			// Compute the time between the current frame and the previous real_time frame.
			offset = time - recordedAnalogSamples[previous].time;
			// Use the relative time to interpolate.
			relative = offset / interval;
			
			for ( int j = 0; j < nChannels; j++ ) {
				samples[smpl].channel[j] = recordedAnalogSamples[previous].channel[j] + 
					relative * ( recordedAnalogSamples[next].channel[j] - recordedAnalogSamples[previous].channel[j] );
			}
		}
	}

	nAcqSamples = smpl;
	return( nAcqSamples );

}

/*********************************************************************************/

bool DexMouseADC::GetCurrentAnalogSample( AnalogSample &sample ) {

	POINT mouse_position;
	GetCursorPos( &mouse_position );
	RECT rect;
	GetWindowRect( GetDesktopWindow(), &rect );

	int chan;

	// Compute 2 voltages from the position of the mouse.
	float y =  (double) (mouse_position.y - rect.top) / (double) ( rect.bottom - rect.top ) * 10.0 - 5.0;
	float x =  (double) (mouse_position.x - rect.right) / (double) ( rect.right - rect.left ) * 10.0 - 5.0;

	sample.time = DexTimerElapsedTime( acquisitionTimer );

	// For the moment, just alternate values between x and y.
	for ( chan = 0; chan < nChannels; chan++ ) {
		if ( chan % 2 ) sample.channel[chan] = x;
		else sample.channel[chan] = y;
	}

	// Output the position and orientation used to compute the simulated
	// marker positions. This is for testing only.
	fprintf( fp, "%f\t", sample.time );
	for ( chan = 0; chan < nChannels; chan++ ) fprintf( fp, "\t%f", sample.channel[chan] );
	fprintf( fp, "\n" );

	return( true );

}



/*********************************************************************************/

int DexMouseADC::Update( void ) {

	// Store the time series of data.
	if ( DexTimerTimeout( acquisitionTimer ) ) {
		acquisitionOn = false;
		overrun =true;
	}
	if ( acquisitionOn && nPolled < DEX_MAX_ANALOG_SAMPLES ) {
		AnalogSample	sample;
		GetCurrentAnalogSample( sample );
		CopyAnalogSample( recordedAnalogSamples[ nPolled ], sample );
		if ( nPolled < DEX_MAX_ANALOG_SAMPLES ) nPolled++;
	}
	return( 0 );

}