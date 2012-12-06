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

// I am also putting here support for calculations on 3D rigid bodies.
// It should probably be a separate class, but I will deal with that later.

#define MAX_RIGID_BODY_MARKERS	256

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
	double ToDegrees( double radians );
	double ToRadians( double degrees );

	void CopyVector( Vector3 destination, const Vector3 source );
	void CopyQuaternion( Quaternion destination, const Quaternion source );
	void VectorsMixin::AddVectors( Vector3 result, const Vector3 a, const Vector3 b );
	void SubtractVectors( Vector3 result, const Vector3 a, const Vector3 b );
	void ScaleVector( Vector3 result, const Vector3 a, const double scaling );

	double VectorNorm( const Vector3 vector );
	void   NormalizeVector( Vector3 v );
	double DotProduct( const Vector3 v1, const Vector3 v2 );
	double AngleBetween( const Quaternion q1, const Quaternion q2 );
	void   ComputeCrossProduct( Vector3 result, const Vector3 v1, const Vector3 v2 );

	void CopyMatrix( Matrix3x3 destination, const Matrix3x3 source );
	void CopyMatrix( double destination[3][3], const Matrix3x3 source );
	void TransposeMatrix( Matrix3x3 destination, const Matrix3x3 source );
	void ScaleMatrix( Matrix3x3 destination, const Matrix3x3 source, const double scaling );
	void MultiplyMatrices( Matrix3x3 result, const Matrix3x3 left, const Matrix3x3 right );

	double Determinant( const	Matrix3x3 m );
	double InvertMatrix( Matrix3x3 result, const Matrix3x3 m );
	void OrthonormalizeMatrix( Matrix3x3 result, Matrix3x3 m );

	void MultiplyVector( Vector3 result, Vector3 v, const Matrix3x3 m );

	void CrossVectors( Matrix3x3 result, const Vector3 left[], const Vector3 right[], int rows );
	void BestFitTransformation( Matrix3x3 result, const Vector3 input[], const Vector3 output[], int rows );
		
	void SetQuaternion( Quaternion result, double radians, const Vector3 axis );
	void SetQuaterniond( Quaternion result, double degrees, const Vector3 axis );
	void NormalizeQuaternion( Quaternion q );
	void MultiplyQuaternions( Quaternion result, const Quaternion q1, const Quaternion q2 );

	void RotateVector( Vector3 result, const Quaternion q, const Vector3 v );
	void MatrixToQuaternion( Quaternion result, Matrix3x3 m );

	bool ComputeRigidBodyPose( Vector3 position, Quaternion orientation,
								Vector3 model[], Vector3 actual[], 
								int N, Quaternion default_orientation );

	char *vstr( const Vector3 v );
	char *qstr( const Quaternion q );
	char *mstr( const Matrix3x3 m );



};


#endif