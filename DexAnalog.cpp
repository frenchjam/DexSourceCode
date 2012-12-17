/*********************************************************************************/
/*                                                                               */
/*                               DexAnalog.cpp                                   */
/*                                                                               */
/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include "..\DexSimulatorApp\resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 
#include <memory.h>
#include <process.h>

#include <ftconfig.h>

#include <fMessageBox.h>
#include <VectorsMixin.h>

#include <DexTimers.h>
#include "DexMonitorServer.h"
#include "Dexterous.h"
#include "DexTargets.h"
#include "DexTracker.h"
#include "DexADC.H"
#include "DexSounds.h"
#include "DexApparatus.h"

/***************************************************************************/

const int DexApparatus::ftAnalogChannel[N_FORCE_TRANSDUCERS] = {0, 6};

void DexApparatus::InitForceTransducers( void ) {

	for ( int sensor = 0; sensor < N_FORCE_TRANSDUCERS; sensor++ ) {
		// Load the calibration data, for calibration index 1.
		// Some transducers might have dual calibrations.
		ftCalibration[sensor] = createCalibration( ATICalFilename[sensor], 1 );
		if ( !ftCalibration[sensor] ) {
			fMessageBox( MB_OK, "DexApparatus", "Unable to load ATI calibration.\n\n Sensor: %d\n Calibration filename: %s", sensor, ATICalFilename[sensor] );
			exit( -1 );
		}
		// Set the units. One should probably check for errors here (see ATI doc).
		SetForceUnits( ftCalibration[sensor], "N" );
		SetTorqueUnits( ftCalibration[sensor], "N-m" );
	}
}

void DexApparatus::ReleaseForceTransducers( void ) {

	for ( int sensor = 0; sensor < N_FORCE_TRANSDUCERS; sensor++ ) {
		if ( ftCalibration[sensor] ) destroyCalibration( ftCalibration[sensor] );
	}

}

/***************************************************************************/

void DexApparatus::ZeroForceTransducers( void ) {

	AnalogSample sample;
	float gauges[N_FORCE_TRANSDUCERS][N_GAUGES];
	int i, j;

	// Take multiple samples and compute an average.
	for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
		for ( j = 0; j < N_GAUGES; j++ ) {
			gauges[i][j] = 0.0;
		}
	}
	for ( i = 0; i < N_SAMPLES_FOR_AVERAGE; i++ ) {
		adc->GetCurrentAnalogSample( sample );
		for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
			for ( j = 0; j < N_GAUGES; j++ ) {
				gauges[i][j] += sample.channel[ftAnalogChannel[i]+j];
			}
		}
		// Perhaps we should sleep here to allow new data to be acquired.
		// Or check when a new sample is taken to see if the timestamp has changed.
	}
	for ( i = 0; i < N_FORCE_TRANSDUCERS; i++ ) {
		for ( j = 0; j < N_GAUGES; j++ ) {
			gauges[i][j] /= (float) N_SAMPLES_FOR_AVERAGE;
		}
	}

	// Average values for the gauges get stored with the calibration.
	for ( int sensor = 0; sensor < N_FORCE_TRANSDUCERS; sensor++ ) {
		Bias( ftCalibration[sensor], gauges[sensor] );
	} 

	// We should signal to ground that this operation has been performed.
	// The average strain gauge values must be saved with the data.

}

/***************************************************************************/

void DexApparatus::ComputeForceTorque(  Vector3 &force, Vector3 &torque, int unit, AnalogSample analog ) {
	
	float ft[6];

	// Use the ATI provided routine to compute force and torque.
	ConvertToFT( ftCalibration[unit], &analog.channel[ftAnalogChannel[unit]], ft );
	// First three components are the force.
	CopyVector( force, ft );
	// Last three are the torque.
	CopyVector( torque, ft + 3 );

}

void DexApparatus::GetForceTorque( Vector3 &force, Vector3 &torque, int unit ) {

	AnalogSample sample;

	adc->GetCurrentAnalogSample( sample );
	ComputeForceTorque( force, torque, unit, sample );

}

/***************************************************************************/

void DexApparatus::ComputeCOP( Vector3 &cop, Vector3 &force, Vector3 &torque ) {
	cop[X] = torque[X] / force[X];
	cop[Y] = torque[Y] / force[Y];
	cop[Z] = 0.0;
}

void DexApparatus::GetCOP( Vector3 &cop, int unit ) {

	Vector3 force;
	Vector3 torque;

	GetForceTorque( force, torque, unit );
	ComputeCOP( cop, force, torque );

}

/***************************************************************************/

float DexApparatus::ComputeGripForce( Vector3 &force1, Vector3 &force2 ) {
	// This is the simple method. Z axes of the two transducers face in 
	// opposite directions. Just take the average of the two, and change sign.
	return(  ( force1[Z] + force2[Z] ) / -2.0 );
}

float DexApparatus::GetGripForce( void ) {
	Vector3 force0, force1;
	Vector3 torque0, torque1;
	GetForceTorque( force0, torque0, 0 );
	GetForceTorque( force1, torque1, 1 );
	return( ComputeGripForce( force0, force1 ) );
}

float DexApparatus::ComputeLoadForce( Vector3 &load, Vector3 &force1, Vector3 &force2 ) {
	// Compute the net force on the object.
	// This is the simple method. 
	// Y and Z axes of the two transducers face in 
	// opposite directions, X axes are aligned.
	// TO DO: Check if I am right about how the transducers are aligned.
	// Is it the X axes that are flipped and the Y axes aligned?
	load[X] = force1[X] + force2[X];
	load[Y] = force1[Y] - force2[Y];
	load[Z] = force1[Z] - force2[Z];
	// Return the magnitude of the net force on the object.
	return( VectorNorm( load ) );
}

float DexApparatus::ComputePlanarLoadForce( Vector3 &load, Vector3 &force1, Vector3 &force2 ) {
	// Compute the net force perpendicular to the pinch axis.
	load[X] = force1[X] + force2[X];
	load[Y] = force1[Y] - force2[Y];
	load[Z] = 0.0;
	// Return the magnitude of the net force on the object.
	return( VectorNorm( load ) );
}

float DexApparatus::GetLoadForce( Vector3 &load ) {
	Vector3 force0, force1;
	Vector3 torque0, torque1;
	GetForceTorque( force0, torque0, 0 );
	GetForceTorque( force1, torque1, 1 );
	return( ComputeLoadForce( load, force0, force1 ) );
}
float DexApparatus::GetPlanarLoadForce( Vector3 &load ) {
	Vector3 force0, force1;
	Vector3 torque0, torque1;
	GetForceTorque( force0, torque0, 0 );
	GetForceTorque( force1, torque1, 1 );
	return( ComputePlanarLoadForce( load, force0, force1 ) );
}
