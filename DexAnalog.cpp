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

#include <ATIDAQ\ftconfig.h>

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
const int DexApparatus::highAccAnalogChannel = HIGH_ACC_CHANNEL;
const int DexApparatus::lowAccAnalogChannel = LOW_ACC_FIRST_CHANNEL;

void DexApparatus::InitForceTransducers( void ) {

	Quaternion align, flip;

	// Set to use the default ATI calibration files.
	// These should be read from a model-specific parameter file.
	ATICalFilename[LEFT_ATI] = DEFAULT_LEFT_ATI_CALFILE;
	ATICalFilename[RIGHT_ATI] = DEFAULT_RIGHT_ATI_CALFILE;

	// Define the rotation of the ATI sensors with respect to the local 
	// manipulandum reference frame. These are probably constants, 
	// but perhaps they should be read from the model-specific parameter file as well.
	ATIRotationAngle[LEFT_ATI] = LEFT_ATI_ROTATION;
	ATIRotationAngle[RIGHT_ATI] = RIGHT_ATI_ROTATION;
	
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
	// TODO: Check what the others are using as a reference frame and make it 
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

// Set the strain gauge offsets in the ATI calibration, so that the offset is nulled when the force is computed.

void DexApparatus::NullifyStrainGaugeOffsets( int unit, float gauge_offsets[N_GAUGES] ) {

	Bias( ftCalibration[unit], gauge_offsets );

	// It could be useful to send the bias values to the ground in real time.
	monitor->SendEvent( "Strain gauge offsets nullified.\n Unit: %d < %f %f %f %f %f %f >", 
		unit, gauge_offsets[0], gauge_offsets[1], gauge_offsets[2], gauge_offsets[3], gauge_offsets[4], gauge_offsets[5] );

	// If someone has been computing filtered force values, they
	// must have gone to zero after this operation.
	filteredGrip = 0.0;
	filteredLoad = 0.0;

}

// Computes the average offset on each of the strain guages and then
// inserts those values into the ATI calibration so that the offset can be 
// compenstated.

// This is the realtime version. It explicitly acquires ADC samples for 
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
		NullifyStrainGaugeOffsets( i, gauges[i] );
		// It could be useful to send the bias values to the ground in real time.
		monitor->SendEvent( "Strain gauge offsets computed.\n Unit: %d < %f %f %f %f %f %f >", 
			i, gauges[i][0], gauges[i][1], gauges[i][2], gauges[i][3],gauges[i][4], gauges[i][5] );
	
	}
}

/***************************************************************************/

void DexApparatus::ComputeForceTorque(  Vector3 &force, Vector3 &torque, int unit, AnalogSample analog ) {
	
	float ft[6] = {0, 0, 0, 0, 0, 0};

	Vector3 ft_force, ft_torque;

	// Use the ATI provided routine to compute force and torque.
	ConvertToFT( ftCalibration[unit], &analog.channel[ftAnalogChannel[unit]], ft );

	// First three components are the force.
	ft_force[X] = ft[0];
	ft_force[Y] = ft[1];
	ft_force[Z] = ft[2];

	// Last three are the torque.
	ft_torque[X] = ft[3];
	ft_torque[Y] = ft[4];
	ft_torque[Z] = ft[5];

	RotateVector( force, ftAlignmentQuaternion[unit], ft_force );
	// TODO: Review the math. I believe that we apply the same
	// rotation to the torque vector as to the force one. But
	// I should think some more to be sure.
	RotateVector( torque, ftAlignmentQuaternion[unit], ft_torque );

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
		// If there is not enough normal force, call it 'invisible'.
		cop[X] = cop[Y] = cop[Z] = INVISIBLE;
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

float DexApparatus::FilteredLoad( float new_load, float filter_constant ) {
	filteredLoad = (new_load + filter_constant * filteredLoad) / (1.0 + filter_constant);
	return( filteredLoad );
}

float DexApparatus::FilteredGrip( float new_grip, float filter_constant ) {
	filteredGrip = (new_grip + filter_constant * filteredGrip) / (1.0 + filter_constant);
	return( filteredGrip );
}