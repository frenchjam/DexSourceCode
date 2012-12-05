/*********************************************************************************/
/*                                                                               */
/*                                   VectorsMixin.cpp                            */
/*                                                                               */
/*********************************************************************************/

// Suport routines for vector and matrix operations.
// Vectors here are 3D. Quaternions are 4D.

#include <stdio.h>
#include <stdlib.h>
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
void VectorsMixin::MultiplyVector( Vector3 result, Vector3 v, const Matrix3x3 m ) {
	// I represent vectors as row vectors so that a matrix is an array of rows.
	// Therefore we normally do right multiplies.
	for ( int i = 0; i < 3; i++ ) {
		result[i] = 0.0;
		for ( int j = 0; j < 3; j++ ) result[i] += v[j] * m[j][i];
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
void VectorsMixin::ScaleMatrix( Matrix3x3 destination, const Matrix3x3 source, const double scaling ){
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			destination[i][j] = scaling * source[i][j];
		}
	}
}
void VectorsMixin::MultiplyMatrices( Matrix3x3 result, const Matrix3x3 left, const Matrix3x3 right ) {
	for ( int i = 0; i < 3; i++ ) {
		for ( int j = 0; j < 3; j++ ) {
			result[i][j] = 0.0;
			for ( int k = 0; k < 3; k++ ) result[i][j] += left[i][k] * right[k][j];
		}
	}
}

// Let left and right be matrices of 3-element row vectors.
// Compute transpose(left) * right, which is necessarily a 3x3 matrix
void VectorsMixin::CrossVectors( Matrix3x3 result, const Vector3 left[], const Vector3 right[], int rows ) {
	// Use doubles to compute the sums, to guard againts underflow.
	double r[3][3];
	int i, j, k;
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			r[i][j] = 0.0;
			for ( k = 0; k < rows; k++ ) r[i][j] += left[k][i] * right[k][j];
		}
	}
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			result[i][j] = r[i][j] / (double) rows;
		}
	}
}

void VectorsMixin::BestFitTransformation( Matrix3x3 result, const Vector3 input[], const Vector3 output[], int rows ) {

	Matrix3x3	left;
	Matrix3x3	right;
	Matrix3x3	left_inverse;

	// This computes the Moore-Penrose pseudo-inverse.
	CrossVectors( right, input, output, rows );
	CrossVectors( left, input, input, rows );
	InvertMatrix( left_inverse, left );
	MultiplyMatrices( result, left_inverse, right );

}

double VectorsMixin::Determinant( const Matrix3x3 m ) {

	return( 
		m[0][0] * ( m[2][2] * m[1][1] - m[2][1] * m[1][2] ) -
		m[1][0] * ( m[2][2] * m[0][1] - m[2][1] * m[0][2] ) +
		m[2][0] * ( m[1][2] * m[0][1] - m[1][1] * m[0][2] ) 
	);

}

double VectorsMixin::InvertMatrix( Matrix3x3 result, const Matrix3x3 m ){

	float r[3][3];
	double det = Determinant( m );

	r[0][0] = m[2][2] * m[1][1] - m[2][1] * m[1][2];
	r[1][0] = m[2][0] * m[1][2] - m[2][2] * m[1][0];
	r[2][0] = m[2][1] * m[1][0] - m[2][0] * m[1][1];

	r[0][1] = m[2][1] * m[0][2] - m[2][2] * m[0][1];
	r[1][1] = m[2][2] * m[0][0] - m[2][0] * m[0][2];
	r[2][1] = m[2][0] * m[0][1] - m[2][1] * m[0][0];

	r[0][2] = m[1][2] * m[0][1] - m[1][1] * m[0][2];
	r[1][2] = m[1][0] * m[0][2] - m[1][2] * m[0][0];
	r[2][2] = m[1][1] * m[0][0] - m[1][0] * m[0][1];

	ScaleMatrix( result, r, 1.0 / det );

	return( det );

}

/*************************************************************************************************/

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
	// To rotate a vector, compute qvq*.
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

double VectorsMixin::AngleBetween( const Quaternion q1, const Quaternion q2 ) {

	Quaternion interim;
	Quaternion conjugate;
	double angle;

	// Compute q1 q2*.
	conjugate[M] = q2[M]; ScaleVector( conjugate, q2, -1.0 );
	MultiplyQuaternions( interim, q1, conjugate );

	// Amplitude of the rotation is the scalar part of the result.
	angle = 2.0 * acos( interim[M] );

	return( angle );

}

/***********************************************************************************/

// These routines create ascii strings from vector and matrix objects.
// They are useful for displaying the values of these object via printf();

char *VectorsMixin::vstr( const Vector3 v ) {

	// This is a circular buffer of static strings.
	// Each time this routine is calle we use a different string to hold the result.
	// That way, if the routine gets called multiple times in a single printf(), 
	//  for example, one call does not write over the output of another.
	// This assumes that we will never need the results of more than 256 at the same time.

	static char str[256][256];
	static instance = 0;
	instance++;
	instance %= 256;

	// Create the string here.
	sprintf( str[instance], "<%8.3f %8.3f %8.3f>", v[X], v[Y], v[Z] );

	return( str[instance] );

}


// The others work in a similar fashion.
char *VectorsMixin::qstr( const Quaternion q ) {
	static char str[256][256];
	static instance = 0;
	instance++;
	instance %= 256;
	sprintf( str[instance], "{%8.3fi + %8.3fj + %8.3fk + %8.3f}", q[X], q[Y], q[Z], q[M] );
	return( str[instance] );
}

char *VectorsMixin::mstr( const Matrix3x3 m ) {
	static char str[256][256];
	static instance = 0;
	instance++;
	instance %= 256;
	sprintf( str[instance], "[%8.3f %8.3f %8.3f | %8.3f %8.3f %8.3f | %8.3f %8.3f %8.3f ]", 
		m[X][X], m[Y][X], m[Z][X], m[X][Y], m[Y][Y], m[Z][Y], m[X][Z], m[Y][Z], m[Z][Z] );
	return( str[instance] );
}

