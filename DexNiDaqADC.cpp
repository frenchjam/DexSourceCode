/***************************************************************************/
/*                                                                         */
/*                                DexNiDaqADC                              */
/*                                                                         */
/***************************************************************************/

// This is the interface to a NiDaq analog input device.

// It is a rudimentary interface that polls the device for single slices of 
// data. It performs time-series acquisitions by repeatedly polling the 
// device as quickly as it can and timestamps the samples. When the caller
// retrieves the samples, the polled data is interpolated to generate time
// series at a constant period.

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <NIDAQmx.h>

#include <fMessageBox.h>

#include <VectorsMixin.h>
#include <DexTimers.h>
#include <Dexterous.h>
#include "DexADC.h"
#include "..\DexSimulatorApp\resource.h"

// #define USE_POLLING

float64	nidaq_buffer[DEX_MAX_CHANNELS * DEX_MAX_ANALOG_SAMPLES];

/*********************************************************************************/

void DexNiDaqADC::ReportNiDaqError ( void ) {
	char errBuff[2048]={'\0'};
	DAQmxGetExtendedErrorInfo( errBuff, 2048);
	fMessageBox( MB_OK, "DexNiDaqADC", errBuff );
}

void DexNiDaqADC::Initialize( void ) {

	acquisitionOn = false; 
	overrun = false;
	nAcqSamples = 0;
	nPolled = 0;

	char channel_range[32];

	char *filename = "DexNiDaqAnalogDebug.adc";
	int32 error_code;

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

	// Define the channels to be acquired.
	// NI-DAQ uses this string based method to select channels. So first
	// I construct the appropriate string based on the number of channels.
	sprintf( channel_range, "Dev1/ai0:%d", nChannels - 1 );

	// Do the real work to initialize the ADC for polling.
	error_code = DAQmxCreateTask("",&taskHandle);
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Set to read single-ended voltages in the range +/- 5 volts.
	error_code = DAQmxCreateAIVoltageChan(taskHandle, channel_range, "", DAQmx_Val_RSE, -5.0, 5.0, DAQmx_Val_Volts, NULL);
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Here we don't set a clock. The read will read all the channels once as quickly as possible.

	// Do the real work to initialize the ADC for polling.
	error_code = DAQmxCreateTask("",&continuousTaskHandle);
	// Set to read single-ended voltages in the range +/- 5 volts.
	error_code = DAQmxCreateAIVoltageChan(continuousTaskHandle, channel_range, "", DAQmx_Val_RSE, -5.0, 5.0, DAQmx_Val_Volts, NULL);
	// Set to acquire at a fixed rate.
	error_code = DAQmxCfgSampClkTiming( continuousTaskHandle, NULL, 1.0 / samplePeriod, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, DEX_MAX_ANALOG_SAMPLES );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();

}

void DexNiDaqADC::Quit( void ) {

	int error_code;

	fclose( fp );

	if( taskHandle!=0 )  {
		// I think that the task stops itself after the read, but I'm not sure.
		// In any case, it doesn't seem to hurt to do this.
		error_code = DAQmxStopTask(taskHandle);
		if( DAQmxFailed( error_code )) ReportNiDaqError();
		// I don't know what happens if you don't clear a task before leaving,
		// but the NI examples says to do it, so I do it.
		error_code = DAQmxClearTask(taskHandle);
		if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	}
	if( continuousTaskHandle!=0 )  {
		// I think that the task stops itself after the read, but I'm not sure.
		// In any case, it doesn't seem to hurt to do this.
		error_code = DAQmxStopTask(continuousTaskHandle);
		if( DAQmxFailed( error_code )) ReportNiDaqError();
		// I don't know what happens if you don't clear a task before leaving,
		// but the NI examples says to do it, so I do it.
		error_code = DAQmxClearTask(continuousTaskHandle);
		if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	}
}



/*********************************************************************************/

void DexNiDaqADC::StartAcquisition( float max_duration ) { 

	acquisitionOn = true; 
	overrun = false;
	DexTimerSet( acquisitionTimer, max_duration );

#ifdef USE_POLLING
	nPolled = 0;
	Update();
#else
	int32		error_code;
	// Start acquiring.
	error_code = DAQmxStartTask( continuousTaskHandle );
	if( DAQmxFailed( error_code )) ReportNiDaqError();


#endif


}

void DexNiDaqADC::StopAcquisition( void ) {
	
	int32       samples_read;
	int32		error_code;

	acquisitionOn = false;
	duration = DexTimerElapsedTime( acquisitionTimer );

#ifndef USE_POLLING
	error_code = DAQmxReadAnalogF64( continuousTaskHandle, DEX_MAX_ANALOG_SAMPLES, 
					0, DAQmx_Val_GroupByScanNumber, 
					nidaq_buffer, DEX_MAX_CHANNELS * DEX_MAX_ANALOG_SAMPLES, 
					&samples_read, NULL );
	// Ignore the timeout error. We don't expect to really get all the samples.
	if( error_code != -200284 && DAQmxFailed( error_code ) ) ReportNiDaqError();
	nAcqSamples = samples_read;

	error_code = DAQmxStopTask(continuousTaskHandle);
	if( DAQmxFailed( error_code )) ReportNiDaqError();
#endif

}

bool DexNiDaqADC::GetAcquisitionState( void ) {
	return( acquisitionOn );
}

bool DexNiDaqADC::CheckAcquisitionOverrun( void ) {
	return( overrun );
}

/*********************************************************************************/

int DexNiDaqADC::RetrieveAnalogSamples( AnalogSample samples[], int max_samples ) {
	
#ifdef USE_POLLING

	int previous;
	int next;
	int smpl;
	
	double	time, interval, offset, relative;
	

	// If we used the polling method to acquire samples, we now
	// need to generate samples at a fixed rate using interpolation.

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

#else

	for ( int smpl = 0; smpl < nAcqSamples; smpl++ ) {
		for ( int chan = 0; chan < nChannels; chan++ ) {
			samples[smpl].channel[chan] = nidaq_buffer[smpl * nChannels + chan];
		}
		samples[smpl].time = smpl * samplePeriod;
	}

#endif


	return( nAcqSamples );

}

/*********************************************************************************/

bool DexNiDaqADC::GetCurrentAnalogSample( AnalogSample &sample ) {

	int32       samples_read;
	int32		error_code;
	float64     nidaqDataSlice[DEX_MAX_CHANNELS];

	// Calculate the size of the buffer in number of samples.
	int32		max_samples = sizeof( nidaqDataSlice ) / sizeof( *nidaqDataSlice );

	// Grab a slice.
	error_code = DAQmxReadAnalogF64( taskHandle, 1, 1.0, DAQmx_Val_GroupByScanNumber, 
		nidaqDataSlice, max_samples, &samples_read, NULL );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Timestamp it.
	sample.time = DexTimerElapsedTime( acquisitionTimer );

	// Copy to buffer..
	for ( int chan = 0; chan < nChannels; chan++ ) {
		sample.channel[chan] = nidaqDataSlice[chan];
	}

	// Output the position and orientation used to compute the simulated
	// marker positions. This is for testing only.
	fprintf( fp, "%f", sample.time );
	for ( chan = 0; chan < nChannels; chan++ ) fprintf( fp, "\t%f", sample.channel[chan] );
	fprintf( fp, "\n" );

	return( true );

}



/*********************************************************************************/

int DexNiDaqADC::Update( void ) {

#if USE_POLLING
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
#endif

	return( 0 );

}