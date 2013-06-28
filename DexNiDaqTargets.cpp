/***************************************************************************/
/*                                                                         */
/*                             DexNiDaqTargets                             */
/*                                                                         */
/***************************************************************************/

// This is the interface to a NiDaq digital outputs.


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
#include "DexTargets.h"



/*********************************************************************************/

void DexNiDaqTargets::ReportNiDaqError ( void ) {
	char errBuff[2048]={'\0'};
	DAQmxGetExtendedErrorInfo( errBuff, 2048);
	fMessageBox( MB_OK, "DexNiDaqADC", errBuff );
}

void DexNiDaqTargets::Initialize( void ) {


	char channel_range[32];
	int32 error_code;

	// Define the channels to be acquired.
	// NI-DAQ uses this string based method to select channels. So first
	// I construct the appropriate string based on the number of channels.
	sprintf( channel_range, "Dev1/port0:2" );

	// Initialize the ports for digital output.
	error_code = DAQmxCreateTask("",&taskHandle);
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Set to read single-ended voltages in the range +/- 5 volts.
	error_code = DAQmxCreateDOChan(taskHandle, channel_range, "", DAQmx_Val_ChanForAllLines );
	if( DAQmxFailed( error_code ) ) ReportNiDaqError();
	// Here we don't set a clock. The read will read all the channels once as quickly as possible.


}

DexNiDaqTargets::DexNiDaqTargets( void ) {

	nVerticalTargets = N_VERTICAL_TARGETS;
	nHorizontalTargets = N_HORIZONTAL_TARGETS;
	nTargets = nVerticalTargets + nHorizontalTargets;

}

void DexNiDaqTargets::SetTargetStateInternal( unsigned long state ) {
	int32 channels_written;
	DAQmxWriteDigitalU32( taskHandle, 1, true, 1.0, DAQmx_Val_GroupByScanNumber, &state, &channels_written, NULL );
}


void DexNiDaqTargets::Quit( void ) {

	int32 error_code;
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

}



