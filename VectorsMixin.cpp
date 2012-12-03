/*********************************************************************************/
/*                                                                               */
/*                                   VectorsMixin.cpp                              */
/*                                                                               */
/*********************************************************************************/

// Suport routines for vector and matrix operations.
// Vectors here are 3D. Quaternions are 4D.

#include <math.h>
#include <VectorsMixin.h>

const int VectorsMixin::X = 0;
const int VectorsMixin::Y = 1;
const int VectorsMixin::Z = 2;
const int VectorsMixin::M = 3;

const double VectorsMixin::pi = 3.14159265358979;

// These are constants that should probably be placed in the Vector3 class.
const Vector3 VectorsMixin::zeroVector = {0.0, 0.0, 0.0};
const Vector3 VectorsMixin::iVector = { 1.0, 0.0, 0.0 };
const Vector3 VectorsMixin::jVector = { 0.0, 1.0, 0.0 };
const Vector3 VectorsMixin::kVector = { 0.0, 0.0, 1.0 };

const Quaternion VectorsMixin::nullQuaternion = {0.0, 0.0, 0.0, 1.0};

const Matrix3x3 VectorsMixin::identityMatrix = {{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
const Matrix3x3 VectorsMixin::zeroMatrix =     {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};

void VectorsMixin::CopyVector( Vector3 destination, const Vector3 source ){
	destination[X] = source[X];
	destination[Y] = source[Y];
	destination[Z] = source[Z];
}
void VectorsMixin::CopyQuaternion( Quaternion destination, const Quaternion source ){
	destination[X] = source[X];
	destination[Y] = source[Y];
	destination[Z] = source[Z];
	destination[M] = source[M];
}
void VectorsMixin::AddVectors( Vector3 result, const Vector3 a, const Vector3 b ) {
	result[X] = a[X] + b[X];
	result[Y] = a[Y] + b[Y];
	result[Z] = a[Z] + b[Z];
}
void VectorsMixin::SubtractVectors( Vector3 result, const Vector3 a, const Vector3 b ) {
	result[X] = a[X] - b[X];
	result[Y] = a[Y] - b[Y];
	result[Z] = a[Z] - b[Z];
}
void VectorsMixin::ScaleVector( Vector3 result, const Vector3 a, const double scaling ) {
	result[X] = (float) a[X] * scaling;
	result[Y] = (float) a[Y] * scaling;
	result[Z] = (float) a[Z] * scaling;
}
void VectorsMixin::MultiplyVector( Vector3 result, const Matrix3x3 m, Vector3 v ) {
	for ( int i = 0; i < 3; i++ ) {
		result[i] = 0.0;
		for ( int j = 0; j < 3; j++ ) result[i] += m[i][j] * v[j];
	}
}

void VectorsMixin::CopyMatrix( Matrix3x3 destination, const Matrix3x3 source ){
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			destination[i][j] = source[i][j];
		}
	}
}

void VectorsMixin::CopyMatrix( double destination[3][3], const Matrix3x3 source ){
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			destination[i][j] = source[i][j];
		}
	}
}

double VectorsMixin::VectorNorm( const Vector3 vector ) {
	return( sqrt( vector[X] * vector[X] + vector[Y] * vector[Y] + vector[Z] * vector[Z] ) );
}

double VectorsMixin::DotProduct( const Vector3 v1, const Vector3 v2 ) {
	return( v1[X] * v2[X] + v1[Y] * v2[Y] + v1[Z] * v2[Z] );
}

void VectorsMixin::MultiplyQuaternions( Quaternion result, const Quaternion q1, const Quaternion q2 ) {

	result[M] = q1[M] * q2[M] - q1[X] * q2[X] - q1[Y] * q2[Y] - q1[Z] * q2[Z];
	result[X] = q1[M] * q2[X] + q1[X] * q2[M] + q1[Y] * q2[Z] - q1[Z] * q2[Y];
	result[Y] = q1[M] * q2[Y] - q1[X] * q2[Z] + q1[Y] * q2[M] + q1[Z] * q2[X];
	result[Z] = q1[M] * q2[Z] + q1[X] * q2[Y] - q1[Y] * q2[X] + q1[Z] * q2[M];

}

void VectorsMixin::RotateVector( Vector3 result, const Quaternion q, const Vector3 v ) {

	Quaternion vq;
	Quaternion rq;
	Quaternion interim;
	Quaternion conjugate;

	// Careful. I'm abusing CopyVector() here.
	// I use it to copy the first 3 components and then I set the 4th.
	vq[M] = 0.0;  CopyVector( vq, v );
	// Now I am abusing ScaleVector to take the conjugate of q.
	conjugate[M] = q[M]; ScaleVector( conjugate, q, -1.0 );

	MultiplyQuaternions( interim, q, vq );
	MultiplyQuaternions( rq, interim, conjugate );

	// Take just the vector part for the result.
	CopyVector( result, rq );

}

void VectorsMixin::SetQuaternion( Quaternion result, double radians, const Vector3 axis ) {

	// Make sure that the specified axis is a unit vector.
	result[M] = (float) cos( 0.5 * radians );
	ScaleVector( result, axis, sin( 0.5 * radians ) / VectorNorm( axis ) );

	Vector3 test;
	RotateVector( test, result, iVector );

}

void VectorsMixin::SetQuaterniond( Quaternion result, double degrees, const Vector3 axis ) {
	SetQuaternion( result, degrees * pi / 180.0, axis );
}



float VectorsMixin::AngleBetween( const Quaternion q1, const Quaternion q2 ) {

	Quaternion interim;
	Quaternion conjugate;
	double angle;

	// Now I am abusing ScaleVector to take the conjugate of q.
	conjugate[M] = q2[M]; ScaleVector( conjugate, q2, -1.0 );
	MultiplyQuaternions( interim, q1, conjugate );

	angle = 2.0 * acos( interim[M] );

	return( angle );

}

