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

	char *filename = "DexMouseAnalogDebug.adc";

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

bool DexMouseADC::CheckAcquisitionOverrun( void ) {
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
				samples[smpl].channel[j] = (float) (recordedAnalogSamples[previous].channel[j] + 
					relative * ( recordedAnalogSamples[next].channel[j] - recordedAnalogSamples[previous].channel[j] ));
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

	// These row vectors effectively invert the gauge-to-force ATI transformations.
	static double Vz[] = { -0.0037,  0.0000, -0.0037,  0.0000, -0.0037,  0.0000 };
	static double Vy[] = {  0.0000,  0.0012,  0.0000, -0.0011,  0.0000, -0.0002 };
	static double Tx[] = { -0.0033, -0.0025, -0.2533,  0.0019,  0.2515, -0.0007 };


	int chan;

	// Compute 2 voltages from the position of the mouse.
	double y =  (double) (mouse_position.y - rect.top) / (double) ( rect.bottom - rect.top ) * 500.0;
	double z =  (double) (mouse_position.x - rect.right) / (double) ( rect.left - rect.right ) * 100.0;
	double t =  pow( ((double) (mouse_position.y - rect.top) / (double) ( rect.bottom - rect.top )), 3.0 );

	sample.time = (float) DexTimerElapsedTime( acquisitionTimer );

	// By default, all values are zero.
	for ( chan = 0; chan < nChannels; chan++ ) sample.channel[chan] = 0.0;
	// Compute analog values to modulate Fz with X and Fy with Y.
	for ( chan = 0; chan < N_GAUGES; chan++ ) {
		sample.channel[ LEFT_ATI_FIRST_CHANNEL + chan] = (float)  (- Vy[chan] * y + Vz[chan] * z - t * Tx[chan]);
		sample.channel[ RIGHT_ATI_FIRST_CHANNEL + chan] = (float) (  Vy[chan] * y + Vz[chan] * z + t * Tx[chan] );
	}

	// Output the simulated values. This is for testing only.
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