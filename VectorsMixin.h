/*********************************************************************************/
/*                                                                               */
/*                                   VectorsMixin.h                              */
/*                                                                               */
/*********************************************************************************/

// Suport routines for vector and matrix operations.
// Vectors here are 3D. Quaternions are 4D.

#ifndef VectorsMixinH
#define VectorsMixinH

typedef float Vector3[3];
typedef float Quaternion[4];
typedef float Matrix3x3[3][3];

class VectorsMixin {

protected:	
	

public:

	static const int X;
	static const int Y;
	static const int Z;
	static const int M;

	static const Vector3 zeroVector;
	static const Quaternion nullQuaternion;
	static const Matrix3x3 identityMatrix;
	static const Matrix3x3 zeroMatrix;

	static const Vector3 iVector;
	static const Vector3 jVector;
	static const Vector3 kVector;

	static const double pi;

	void CopyVector( Vector3 destination, const Vector3 source );
	void CopyQuaternion( Quaternion destination, const Quaternion source );
	void VectorsMixin::AddVectors( Vector3 result, const Vector3 a, const Vector3 b );
	void SubtractVectors( Vector3 result, const Vector3 a, const Vector3 b );
	void ScaleVector( Vector3 result, const Vector3 a, const double scaling );
	void MultiplyVector( Vector3 result, const Matrix3x3 m, Vector3 v );

	void CopyMatrix( Matrix3x3 destination, const Matrix3x3 source );
	void CopyMatrix( double destination[3][3], const Matrix3x3 source );

	double VectorNorm( const Vector3 vector );
	double DotProduct( const Vector3 v1, const Vector3 v2 );
	float AngleBetween( const Quaternion q1, const Quaternion q2 );

	void MultiplyQuaternions( Quaternion result, const Quaternion q1, const Quaternion q2 );
	void RotateVector( Vector3 result, const Quaternion q, const Vector3 v );

	void SetQuaternion( Quaternion result, double radians, const Vector3 axis );
	void SetQuaterniond( Quaternion result, double degrees, const Vector3 axis );


};


#endif