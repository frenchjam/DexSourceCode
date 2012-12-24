/*********************************************************************************/
/*                                                                               */
/*                               DexAnalog.cpp                                   */
/*                                                                               */
/*********************************************************************************/

// Methods to process the analog data, notably the ATI data.

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

const int DexApparatus::ftAnalogChannel[N_FORCE_TRANSDUCERS] = {LEFT_ATI_FIRST_CHANNEL, RIGHT_ATI_FIRST_CHANNEL};

void DexApparatus::InitForceTransducers( void ) {

	Quaternion align, flip;

	for ( int sensor = 0; sensor < nForceTransducers; sensor++ ) {
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

	// Compute the transformations to put ATI forces in a common reference frame.
	// TO DO: Check what the others are using as a reference frame and make it 
	// coherent. Ideally, the local manipulandum reference frame should align with
	// the world reference frame when the manipulandum is held upright in the seated posture.
	SetQuaterniond( ftAlignmentQuaternion[0], ATIRotationAngle[0], kVector );
	SetQuaterniond( align, ATIRotationAngle[1], kVector );
	SetQuaterniond( flip, 180.0, iVector );
	MultiplyQuaternions( ftAlignmentQuaternion[1], flip, align );
}

void DexApparatus::ReleaseForceTransducers( void ) {

	for ( int sensor = 0; sensor < nForceTransducers; sensor++ ) {
		if ( ftCalibration[sensor] ) destroyCalibration( ftCalibration[sensor] );
	}

}

/***************************************************************************/

// Computes the average offset on each of the strain guages and then
// inserts those values into the ATI calibration so that the offset can be 
// compenstated.

// This is the realtime version. It specifically acquires ADC samples for 
// this purpose. See also the post hoc version that works on an acquired buffer
// full of analog data.

void DexApparatus::ZeroForceTransducers( void ) {

	AnalogSample sample;
	float gauges[N_FORCE_TRANSDUCERS][N_GAUGES];
	int i, j, k;

	float previous_timestamp = -1.0;

	// Take multiple samples and compute an average.
	// First zero the sums.
	for ( i = 0; i < nForceTransducers; i++ ) {
		for ( j = 0; j < nGauges; j++ ) {
			gauges[i][j] = 0.0;
		}
	}
	// Accumulate the specified number of samples for the average.
	for ( i = 0; i < nSamplesForAverage; i++ ) {
		// Make sure that we take new samples.
		do {
			adc->GetCurrentAnalogSample( sample );
		} while ( sample.time == previous_timestamp );
		previous_timestamp = sample.time;
		// Add the new samples to the sums.
		for ( j = 0; j < nForceTransducers; j++ ) {
			for ( k = 0; k < N_GAUGES; k++ ) {
				gauges[j][k] += sample.channel[ftAnalogChannel[j]+k];
			}
		}
	}
	for ( i = 0; i < nForceTransducers; i++ ) {
		for ( j = 0; j < N_GAUGES; j++ ) {
			gauges[i][j] /= (float) N_SAMPLES_FOR_AVERAGE;
		}
	}

	// Average values for the gauges get stored with the calibration.
	for ( int sensor = 0; sensor < nForceTransducers; sensor++ ) {
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
	// We pretend that FT is a vector.
	RotateVector( force, ftAlignmentQuaternion[unit], ft );
	// Last three are the torque. I acces the last 3 values like a vector.
	// TO DO: Review the math. I believe that we apply the same
	// rotation to the torque vector as to the force one. But
	// I should think some more to be sure.
	RotateVector( torque, ftAlignmentQuaternion[unit], ft + 3 );

}

void DexApparatus::GetForceTorque( Vector3 &force, Vector3 &torque, int unit ) {

	AnalogSample sample;

	adc->GetCurrentAnalogSample( sample );
	ComputeForceTorque( force, torque, unit, sample );

}

/***************************************************************************/

double DexApparatus::ComputeCOP( Vector3 &cop, Vector3 &force, Vector3 &torque, double threshold ) {
	// If there is enough normal force, compute the center of pressure.
	if ( fabs( force[Z] ) > threshold ) {
		cop[X] = - torque[Y] / force[Z];
		cop[Y] = - torque[X] / force[Z];
		cop[Z] = 0.0;
		// Return the distance from the center.
		return( sqrt( cop[X] * cop[X] + cop[Y] * cop[Y] ) );
	}
	else {
		// If there is not enough normal force, just call it a centered COP.
		CopyVector( cop, zeroVector );
		// But signal that it's not a valid COP by returning a negative distance.
		return( -1 );
	}
}

double DexApparatus::GetCOP( Vector3 &cop, int unit, double threshold ) {

	Vector3 force;
	Vector3 torque;

	GetForceTorque( force, torque, unit );
	return( ComputeCOP( cop, force, torque, threshold ) );

}

/***************************************************************************/

float DexApparatus::ComputeGripForce( Vector3 &force1, Vector3 &force2 ) {
	// Force readings have been transformed into a common reference frame.
	// Take the average of two opposing forces.
	return(  ( force1[Z] - force2[Z] ) / 2.0 );
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
	// If the reference frames have been properly aligned,
	// the net force on the object is simply the sum of the forces 
	// applied to either side.
	load[X] = force1[X] + force2[X];
	load[Y] = force1[Y] + force2[Y];
	load[Z] = force1[Z] + force2[Z];
	// Return the magnitude of the net force on the object.
	return( VectorNorm( load ) );
}

float DexApparatus::ComputePlanarLoadForce( Vector3 &load, Vector3 &force1, Vector3 &force2 ) {
	// Compute the net force perpendicular to the pinch axis.
	ComputeLoadForce( load, force1, force2 );
	// Ignore the Z component.
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
