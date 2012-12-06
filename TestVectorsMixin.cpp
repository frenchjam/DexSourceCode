// TestVectorsMixin.cpp

#include <stdio.h>
#include <stdlib.h>
#include <VectorsMixin.h>
#include <math.h>

double noise = 0.0;

Matrix3x3	m = {{1.0, 0.0, 0.0}, {0.0, 2.0, 0.0}, {0.0, 0.0, 3.0}};
Vector3		v = {1.0, 1.0, 1.0};

Vector3		input[8] = 
{
	{1.0, 0.0, 0.0}, 
	{0.0, 1.0, 0.0},  
	{0.0, 0.0, 1.0}, 
	{1.0, 1.0, 0.0}, 
	{1.0, 0.0, 1.0}, 
	{0.0, 1.0, 1.0}, 
	{1.0, 1.0, 1.0},  
	{2.0, 2.0, 2.0}
};
int N = 8;
Quaternion rotation;
Vector3		output[8];

Matrix3x3	best;

// Compute a random number between -1 and +1.
// It doesn't need to be perfect.
double random() {
	return( rand() / 32767.0 * 2.0 - 1.0 );
}

int main( int argc, char *argv[] ) {

	VectorsMixin	vm;

	Vector3		mv;
	Matrix3x3  inv;
	Matrix3x3  res;
	Matrix3x3  ortho;
	Matrix3x3  transpose;
	Quaternion	q;

	Vector3 check;
	Vector3 delta;
	double	error;


	int i, j;

	vm.SetQuaternion( q, vm.pi / 6, vm.iVector );
	vm.RotateVector( check, q, input[7] );


	printf( "\n**********************************************************************\n\n" );
	printf( "\nTest matrix multiplication.\n\n" );
	// Multiply the vector by the matrix (row vectors -> right multiply!).
	vm.MultiplyVector( mv, v, m );
	printf( "v:   %s\nM:   %s\nM*v: %s\n", vm.vstr( v ), vm.mstr( m ), vm.vstr( mv ) );

	// Test the matrix inversion. Here we test it on the simple diagonal matrix.
	printf( "\nTest matrix inversion.\n\n" );
	printf( "M:          %s\n", vm.mstr( m ) );
	printf( "Det(M):     %f\n", vm.Determinant( m ) );
	vm.InvertMatrix( inv, m );
	printf( "Det(Minv):  %f\n",  vm.Determinant( inv ) );
	printf( "Minv:       %s\n\n", vm.mstr( inv ) );
 		
	// Create a set of random matrices.
	// Invert each one and multiply them together.
	// The result should be the identity matrix, with determinant = 1.
	for ( int n = 0; n < 10; n++ ) {
		for ( i = 0; i < 3; i++ ) {
			for ( j = 0; j < 3; j++ ) {
				m[i][j] = random();
			}
		}
		printf( "M:          %s\n", vm.mstr( m ) );
		vm.InvertMatrix( inv, m );
		printf( "Minv:       %s\n", vm.mstr( inv ) );
		vm.MultiplyMatrices( res, m, inv );
		printf( "Mres:       %s\n", vm.mstr( res ) );
		printf( "Det(M): %f  Det(Minv): %f  Det( M * Minv ): %f\n\n", 
			vm.Determinant( m ), vm.Determinant( inv ), vm.Determinant( m ) * vm.Determinant( inv) );
	}

	printf( "\n**********************************************************************\n\n" );
	printf( "Test othonormality.\n\n" );

	// See if we can construct an orthonormal matrix from an arbitrary matrix.
	vm.OrthonormalizeMatrix( ortho, m );
	vm.TransposeMatrix( transpose, ortho );
	vm.MultiplyMatrices( res, ortho, transpose );
	printf( "M:          %s\n", vm.mstr( m ) );
	printf( "Ortho:      %s\n", vm.mstr( ortho ) );
	printf( "Transpose:  %s\n", vm.mstr( transpose ) );
	printf( " O * T      %s\n\n", vm.mstr( res ) );
 
	for ( i = 0; i < 3; i++ ) {
		for ( j = 0; j < 3; j++ ) {
			m[i][j] = random();
		}
	}

			
	vm.OrthonormalizeMatrix( ortho, m );
	vm.TransposeMatrix( transpose, ortho );
	vm.MultiplyMatrices( res, ortho, transpose );
	printf( "M:          %s\n", vm.mstr( m ) );
	printf( "Ortho:      %s\n", vm.mstr( ortho ) );
	printf( "Transpose:  %s\n", vm.mstr( transpose ) );
	printf( " O * T      %s\n", vm.mstr( res ) );

	printf( "\n**********************************************************************\n\n" );
	printf( "Test quaternion calculations.\n\n" );
	Quaternion q1, q2;
	double angle1 = 180.0 * random();
	double angle2 = 180.0 * random();
	// Compute a random rotation transformation.
	Vector3	axis;
	// Random axis;
	for ( j = 0; j < 3; j++ ) axis[j] = random();
	// Make it a unit vector.
	vm.NormalizeVector( axis );
	vm.SetQuaterniond( q1, angle1, axis );
	vm.SetQuaterniond( q2, angle2, axis );
	printf( "Axis %s\nAngle 1: %f  Angle 2: %f\nDifference: %f Angle Between: %f  Error: %f\n\n",
		vm.vstr( axis ), angle1, angle2, 
		fabs( angle1 - angle2 ), vm.ToDegrees( vm.AngleBetween( q1, q2 ) ),
		fabs( angle1 - angle2 - vm.ToDegrees( vm.AngleBetween( q1, q2 ) ) )
		);

	printf( "\n**********************************************************************\n\n" );
	printf( "Test orientation identification algorithm.\n\n" );

	// Test the algorithm for finding the orientation of the manipulandum.
	// Finding the orientation consists of finding the best-fit rotation matrix
	//  that maps the model marker locations (input) to the measured marker locations (output).
	// First we test the method for finding the best-fit transformation for the manipulandum at zero.

	// Compute a random rotation transformation.
	// Random axis;
	for ( j = 0; j < 3; j++ ) axis[j] = random();
	// Make it a unit vector.
	vm.ScaleVector( axis, axis, 1.0 / vm.VectorNorm( axis ) );
	// Random angle.
	double angle = vm.pi * random();
	vm.SetQuaterniond( rotation, angle, axis );
	printf( "Angle: %f  Axis: %s  Quaternion: %s\n\n",
		angle, vm.vstr( axis ), vm.qstr( rotation ) );

	// Compute the equivalent rotation matrix directly by rotating the basis vectors.
	for ( i = 0; i < 3; i++ ) vm.RotateVector( best[i], rotation, vm.identityMatrix[i] );
	// Compute the quaternion from that matrix.
	vm.MatrixToQuaternion( q, best );
	printf( "Matrix from q: %s %f\n", vm.mstr( best ), vm.Determinant( best ) );
	printf( "Original quaternion:    %s\n", vm.qstr( rotation ) );
	printf( "Quaternion from matrix: %s\n", vm.qstr( q ) );
	printf( "Angular difference:     %f\n\n", vm.ToDegrees( vm.AngleBetween( rotation, q )) );

	// Transform the set of input vectors.
	for ( i = 0; i < N; i++ ) {
		vm.RotateVector( output[i], rotation, input[i] );
		// Add some noise.
		for ( j = 0; j < 3; j++ ) output[i][j] += noise * random();
	}
	// Compute the best-fit transformation that maps input to output.
	vm.BestFitTransformation( best, input, output, N );
	vm.MatrixToQuaternion( q, best );
	printf( "Best fit: %s %f\n", vm.mstr( best ), vm.Determinant( best ) );
	printf( "Original quaternion: %s\n", vm.qstr( rotation ) );
	printf( "Best-fit quaternion: %s\n", vm.qstr( q ) );
	printf( "Anglular difference: %f\n\n", vm.ToDegrees( vm.AngleBetween( rotation, q ) ) );


//	printf( "\nPress <RETURN> to continue ..." );
//	getchar();
//	exit( 0 );

	///////////////////////////////////////////////////////////////////////
	
	for ( i = 0; i < N; i++ ) {
		Vector3 check;
		Vector3 delta;
		double	error;
		double theta;

		vm.MultiplyVector( check, input[i], best );
		vm.SubtractVectors( delta, output[i], check );
		error = vm.VectorNorm( delta );
		theta = 180.0 / vm.pi * acos( vm.DotProduct( output[i], check ) / vm.VectorNorm( output[i] ) / vm.VectorNorm( check ) );
			printf( "Check: %d %s %s %f %f\n", i, vm.vstr( output[i] ), vm.vstr( check ), error, theta );
	}

	// Now apply this method to a manipulandum that has been translated and rotated.
	// First we compute a new set of output vectors by adding a 3D displacement.
	// Then we try to compute the best-fit rotation and translation that maps input to output.

	printf( "\nCheck for different combinations of markers.\n\n" );

	Vector3 displacement = {100.0, 200, 3000.0};
	for ( i = 0; i < N; i++ ) vm.AddVectors( output[i], output[i], displacement );

	Vector3 selected_input[8];
	Vector3 selected_output[8];
	Vector3 delta_input[8];
	Vector3 delta_output[8];
	Vector3 input_centroid;
	Vector3 output_centroid;
	double count;
	int n_sel;

	for ( int combo = 0; combo < 256; combo++ ) {

		// Select the visible output markers and the corresponding inputs.
		count = 0.0;
		n_sel = 0;
		for ( i = 0; i < N; i++ ) {
			if ( ( 0x01 << i ) & combo ) {
				vm.CopyVector( selected_input[n_sel], input[n_sel] );
				vm.CopyVector( selected_output[n_sel], output[n_sel] );
				count++;
				n_sel++;
			}
		}

		// Compute the centroid of the input and output vectors;
		vm.CopyVector( input_centroid, vm.zeroVector );
		vm.CopyVector( output_centroid, vm.zeroVector );
		for ( i = 0; i < n_sel; i++ ) {
			vm.AddVectors( input_centroid, input_centroid, selected_input[i] );
			vm.AddVectors( output_centroid, output_centroid, selected_output[i] );
		}
		vm.ScaleVector( input_centroid, input_centroid, 1.0 / count );
		vm.ScaleVector( output_centroid, output_centroid, 1.0 / count );

		// Now compute the vector offset of each marker from the centroid.
		for ( i = 0; i < n_sel; i++ ) {
			vm.SubtractVectors( delta_input[i], selected_input[i], input_centroid );
			vm.SubtractVectors( delta_output[i], selected_output[i], output_centroid );
		}

		// Apply the best-fit method that we tested above to the deltas.
		// Note that this will give a valid result only if there are at 
		// least 4 markers visible. Another method must be used if there
		// are only 3 visible markers. It's not possible to compute the 
		// orientation if there are just 2 or 1, but we could calculate a
		// position assuming a zero orientation. Here I just show the results
		// of the best fit solution.

		// TO DO: Implement a solution for 3, 2 or 1 markers.
		vm.BestFitTransformation( best, delta_input, delta_output, (int) count );

		// Now compute the displacement.
		Vector3 average_displacement;
		vm.CopyVector( average_displacement, vm.zeroVector );
		for ( i = 0; i < n_sel; i++ ) {
			Vector3 rotated_input;
			Vector3 delta;

			vm.MultiplyVector( rotated_input, selected_input[i], best );
			vm.SubtractVectors( delta, selected_output[i], rotated_input );
			vm.AddVectors( average_displacement, average_displacement, delta );
		}
		vm.ScaleVector( average_displacement, average_displacement, 1.0 / count );
		vm.SubtractVectors( delta, displacement, average_displacement );

		vm.MatrixToQuaternion( q, best );
//		printf( "\nBest fit: %s %f\n", vm.mstr( best ), vm.Determinant( best ) );
//		printf( "Original quaternion: %s\n", vm.qstr( rotation ) );
//		printf( "Best-fit quaternion: %s\n", vm.qstr( q ) );
//		printf( "Anglular difference: %f\n", vm.ToDegrees( vm.AngleBetween( rotation, q ) ) );

		for ( i = 0; i < n_sel; i++ ) {

			vm.MultiplyVector( check, selected_input[i], best );
			vm.AddVectors( check, check, average_displacement );
			vm.SubtractVectors( delta, selected_output[i], check );
			error = vm.VectorNorm( delta );
//			printf( "Check: %d %s %s %f\n", i, vm.vstr( selected_output[i] ), vm.vstr( check ), error );
		}

		// Construct a visibility string.
		char visibility[16] = "--------";
		for ( i = 7, j = 0; i >= 0; i--, j++ ) {
			if ( (0x01 << j) & combo ) visibility[i] = 'O';
		}
		printf( "%s %s %s %f\n", visibility, vm.vstr( displacement), vm.vstr( average_displacement ), vm.VectorNorm( delta ) );
	}

	/***********************************************************************************************/

	//
	// Repeat the above, using ComputeRigidBodyPose().
	//

	Vector3		estimated_displacement;
	Quaternion	estimated_rotation;
	bool		visible;

	printf( "\nComputeRigidBodyPose():\n\n" );

	// Try with all possible combinations of 8 markers.
	for ( combo = 0; combo < 256; combo += 1 ) {

		// Select the visible output markers and the corresponding inputs.
		int n_markers = 0;
		for ( i = 0; i < N; i++ ) {
			if ( ( 0x01 << i ) & combo ) {
				vm.CopyVector( selected_input[n_markers], input[n_markers] );
				vm.CopyVector( selected_output[n_markers], output[n_markers] );
				n_markers++;
			}
		}

		// ComputeRigidBodyPose() does it all!
		visible = vm.ComputeRigidBodyPose( estimated_displacement, estimated_rotation, selected_input, selected_output, n_markers, NULL );
		
		// Compute the residual error with the ideal values of position (displacement) and orientation (rotation).
		vm.SubtractVectors( delta, estimated_displacement, displacement );
		double displacement_residual = vm.VectorNorm( delta );
		double rotation_residual = vm.ToDegrees( vm.AngleBetween( rotation, estimated_rotation ) );


		// Construct a visibility string.
		char visibility[16] = "--------";
		for ( i = 7, j = 0; i >= 0; i--, j++ ) {
			if ( (0x01 << j) & combo ) visibility[i] = 'O';
		}
		printf( "%s %s %s %s %f %f\n", 
			visibility, vm.vstr( displacement), vm.vstr( estimated_displacement ), 
			( visible ? "YES" : "NO " ), displacement_residual, rotation_residual );
	}

	printf( "\nPress <RETURN> to continue ..." );
	fflush( stdout );
	getchar();



	return( 0 );

}