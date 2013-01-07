/*********************************************************************************/
/*                                                                               */
/*                                   DexApparatus.cpp                            */
/*                                                                               */
/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include "..\DexSimulatorApp\resource.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h> 

#include <VectorsMixin.h>

#include "DexApparatus.h"
#include "Dexterous.h"

/*********************************************************************************/
/*                                                                               */
/*                                Post Hoc Testing                               */
/*                                                                               */
/*********************************************************************************/

//
// Data acquisitions have to have some limit on the time span to avoid overunning 
//  the allocated buffers. Normally, it will be large enough to handle the nominal
//  conditions, but user actions could make it go longer. Here we check if an 
//  overrun occured to see if we acquired the full task.
//

int DexApparatus::CheckOverrun(  const char *msg ) {
	
	if ( tracker->CheckAcquisitionOverrun() ) {
		monitor->SendEvent( "Acquisition overrun." );
		
		int response = SignalError( MB_ABORTRETRYIGNORE, msg );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// Log the successful overrun check.
	monitor->SendEvent( "Overrun check OK." );
	return( NORMAL_EXIT );
	
}

//
// Allows to check if the manipulandum goes out of view too often during the trial.
// There are two thresholds, one on the overall time during the trial that the 
//  manipulandum can be invisible, the other on the maximum gap in the data.
//

int DexApparatus::CheckVisibility(  double max_cumulative_dropout_time, 
								    double max_continuous_dropout_time,
								    const char *msg ) {

	int first, last;
	
	// Arguments are in seconds, but it's easier to work in samples.
	int max_dropout_samples = max_continuous_dropout_time / tracker->GetSamplePeriod();
	
	// Count the number of samples where the manipulandum is invisible.
	int overall = 0;
	int continuous = 0;
	
	// Keep track of the longest continuous dropout.
	int max_gap = 0;
	bool interval_exceeded = false;

	// Limit the range of frames used in the analysis, if specified in the script.
	FindAnalysisFrameRange( first, last );
	
	for ( int i = first; i < last; i ++ ) {
		if ( ! acquiredManipulandumState[i].visibility ) {
			overall++;
			continuous++;
			if ( continuous > max_dropout_samples ) interval_exceeded = true;
			if ( continuous > max_gap ) max_gap = continuous;
		}
		else {
			continuous = 0;
		}
	}
	// Compute the duration of the continuous and cumulative gaps in seconds.
	double overall_time = overall * tracker->GetSamplePeriod();
	double max_time_gap = max_gap * tracker->GetSamplePeriod();
	
	// If the criteria for acceptable dropouts is exceeded, signal the error.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( interval_exceeded ||  overall_time >= max_cumulative_dropout_time ) {
		
		const char *fmt;

		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "Manipulandum Visibility Error.";
		fmt = "%s\n  Total Time Invisible:   %.3f (%.3f)\n  Longest Gap:             %.3f (%.3f)";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg,
			overall_time, max_cumulative_dropout_time, max_time_gap, max_continuous_dropout_time );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	monitor->SendEvent( "Visibility Check OK - Max Gap: %.3f Cumulative: %.3f", max_time_gap, overall_time );
	return( NORMAL_EXIT );
	
}

/********************************************************************************************/

//
// Checks if there was sufficient movement along the required axis.
// Here we base the test on the computed standard deviation along each axis.
// This measurement is less susceptible to noisy data and outliers, compared to computing
//  the max and min value along each axis. But it will take some work to define the appropriate
//  threshold values for each of the protocols. For instance, what would be the expected SD for 
//  a set of targeted movements?

int DexApparatus::CheckMovementAmplitude(  float min, float max, 
										   float dirX, float dirY, float dirZ,
										   const char *msg ) {
	
	const char *fmt;
	bool  error = false;
	
	int i, k, m;

	int first, last;
	
	double N = 0.0, Sxy[3][3], sd;
	Vector3 delta, mean, direction, vect;

	// TO DO: Should normalize the direction vector here.
	direction[X] = dirX;
	direction[Y] = dirY;
	direction[Z] = dirZ;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Compute the mean position. 
	N = 0.0;
	CopyVector( mean, zeroVector );
	for ( i = first; i < last; i++ ) {
		if ( acquiredManipulandumState[i].visibility ) {
			if ( abs( acquiredManipulandumState[i].position[X] ) > 1000 ) {
				i = i;
			}
			N++;
			AddVectors( mean, mean, acquiredManipulandumState[i].position );
		}
	}
	// If there is no valid position data, signal an error.
	if ( N <= 0.0 ) {
		monitor->SendEvent( "Movement extent - No valid data." );
		sd = 0.0;
		error = true;
	}
	else {
	
		// This is the mean.
		ScaleVector( mean, mean, 1.0 / N );;

		// Compute the sums required for the variance calculation.
		CopyMatrix( Sxy, zeroMatrix );
		N = 0.0;

		for ( i = first; i < last; i ++ ) {
			if ( acquiredManipulandumState[i].visibility ) {
				N++;
				SubtractVectors( delta, acquiredManipulandumState[i].position, mean );
				for ( k = 0; k < 3; k++ ) {
					for ( m = 0; m < 3; m++ ) {
						Sxy[k][m] += delta[k] * delta[m];
					}
				}
			}
		}
		
		// If we have some data, compute the directional variance and then
		// the standard deviation along that direction;
		// This is just a scalar times a matrix.

		// Sxy has to be a double or we risk underflow when computing the sums.
		// The Vectors package is for float vectors, so we implement the operations
		//  directly here.
		// TO DO: Implement vector function for double matrices.
		for ( k = 0; k < 3; k++ ) {
			vect[k] = 0;
			for ( m = 0; m < 3; m++ ) {
				Sxy[k][m] /= N;
			}
		}
		// This is a matrix multiply.
		// TO DO: Implement vector function for double matrices.
		for ( k = 0; k < 3; k++ ) {
			for ( m = 0; m < 3; m++ ) {
				vect[k] += Sxy[m][k] * direction[m];
			}
		}
		// Compute the length of the vector, which is the variance along 
		// the specified direction. Then take the square root of that 
		// magnitude to get the standard deviation along that direction.
		sd = sqrt( VectorNorm( vect ) );

		// Check if the computed value is in the desired range.
		error = ( sd < min || sd > max );

	}

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "Movement extent outside range.";
		// The message could be different depending on whether the maniplulandum
		// was not moved enough, or if it just could not be seen.
		if ( N <= 0.0 ) fmt = "%s\n Manipulandum not visible.";
		else fmt = "%s\n Measured: %f\n Desired range: %f - %f\n Direction: < %.2f %.2f %.2f>";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, sd, min, max, dirX, dirY, dirZ );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event.
	monitor->SendEvent( "Movement extent OK.\n Measured: %f\n Desired range: %f - %f\n Direction: < %.2f %.2f %.2f>", sd, min, max, dirX, dirY, dirZ );
	return( NORMAL_EXIT );
	
}

// Same as above, but with a array as an input to specify the direction.
int DexApparatus::CheckMovementAmplitude(  float min, float max, const float direction[3], char *msg ) {
	return ( CheckMovementAmplitude( min, max, direction[X], direction[Y], direction[Z], msg ) );
}

/********************************************************************************************/

//
// Checks the number of oscillations in the specified direction.
// Cycles are counted by counting the number of zero crossings in the
// positive direction. The hysteresis parameter is used to reject noise.
// 

int DexApparatus::CheckMovementCycles(  int min_cycles, int max_cycles, 
										   float dirX, float dirY, float dirZ,
										   float hysteresis, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	float	displacement = 0.0;
	bool	positive = false;
	int		cycles = 0;

	Vector3 direction, mean, delta;
	
	int i;

	int first, last;
	
	double N = 0.0;

	// Just make sure that the user gave a positive value for hysteresis.
	hysteresis = fabs( hysteresis );

	// TO DO: Should normalize the direction vector here.
	direction[X] = dirX;
	direction[Y] = dirY;
	direction[Z] = dirZ;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );
	for ( first = first; first < last; first++ ) if ( acquiredManipulandumState[first].visibility ) break;

	// Compute the mean position. 
	N = 0.0;
	CopyVector( mean, zeroVector );
	for ( i = first; i < last; i++ ) {
		if ( acquiredManipulandumState[i].visibility ) {
			N++;
			AddVectors( mean, mean, acquiredManipulandumState[i].position );
		}
	}
	// If there is no valid position data, signal an error.
	if ( N <= 0.0 ) {
		monitor->SendEvent( "Movement cycles - No valid data." );
		cycles = 0;
		error = true;
	}
	else {
	
		// This is the mean.
		ScaleVector( mean, mean, 1.0 / N );

		// Step through the trajectory, counting positive zero crossings.
		for ( i = first; i < last; i ++ ) {
			// Compute the displacements around the mean.
			if ( acquiredManipulandumState[i].visibility ) {
				SubtractVectors( delta, acquiredManipulandumState[i].position, mean );
				displacement = DotProduct( delta, direction );
			}
			// If on the positive side of the mean, just look for the negative transition.
			if ( positive ) {
				if ( displacement < - hysteresis ) positive = false;
			}
			// If on the negative side, look for the positive transition.
			// If we find one, count another cycle.
			else {
				if ( displacement > hysteresis ) {
					positive = true;
					cycles++;
				}
			}
		}

		// Check if the computed number of cycles is in the desired range.
		error = ( cycles < min_cycles || cycles > max_cycles );

	}

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "Movement cycles outside range.";
		// The message could be different depending on whether the maniplulandum
		// was not moved enough, or if it just could not be seen.
		if ( N <= 0.0 ) fmt = "%s\n Manipulandum not visible.";
		else fmt = "%s\n Measured cycles: %d\n Desired range: %d - %d\n Direction: < %.2f %.2f %.2f>";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, cycles, min_cycles, max_cycles, dirX, dirY, dirZ );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event.
	monitor->SendEvent( "Movement extent OK.\n Measured: %f\n Desired range: %f - %f\n Direction: < %.2f %.2f %.2f>", cycles, min_cycles, max_cycles, dirX, dirY, dirZ );
	return( NORMAL_EXIT );
	
}

// Same as above, but with a array as an input to specify the direction.
int DexApparatus::CheckMovementCycles(  int min_cycles, int max_cycles, const Vector3 direction, float hysteresis, const char *msg ) {
	return ( CheckMovementCycles( min_cycles, max_cycles, direction[X], direction[Y], direction[Z], hysteresis, msg ) );
}

/********************************************************************************************/

//
// Checks that the tangential velocity is zero when the cue to start a movement occurs.
// Movement triggers must be marked by LogEvent( TRIGGER_MOVEMENT );
// 
// TO DO: Not yet tested!

int DexApparatus::CheckEarlyStarts(  int max_early_starts, float hold_time, float threshold, float filter_constant, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	int		early_starts = 0;
	int		first, last;
	int		i, j, index, frm;
	int		hold_frames = (int) floor( hold_time / tracker->samplePeriod );

	int N = 0;
	double tangential_velocity[DEX_MAX_MARKER_FRAMES];
	Vector3 delta;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Compute the instantaneous tangential velocity. 
	for ( i = first + 1; i < last; i++ ) {
		if ( acquiredManipulandumState[i].visibility && acquiredManipulandumState[i-1].visibility) {
			SubtractVectors( delta, acquiredManipulandumState[i].position, acquiredManipulandumState[i-1].position );
			ScaleVector( delta, delta, 1.0 / tracker->GetSamplePeriod() );
			tangential_velocity[i] = VectorNorm( delta );
			N++;
		}
		else {
			tangential_velocity[i] = tangential_velocity[i-1];
		}
	}
	// If there is no valid position data, signal an error.
	if ( N <= 0.0 ) {
		monitor->SendEvent( "No valid data." );
		error = true;
	}
	else {

		// Smooth the tangential velocity using a recursive filter.
		for ( frm = 1; frm < nAcqFrames; frm++ ) {
			tangential_velocity[frm] = ( filter_constant * tangential_velocity[frm-1] + tangential_velocity[frm] ) / ( 1.0 + filter_constant );
		}
		// Run the filter backwards to eliminate the phase lag.
		for ( frm = nAcqFrames - 2; frm >= 0; frm-- ) {
			tangential_velocity[frm] = ( filter_constant * tangential_velocity[frm+1] + tangential_velocity[frm] ) / ( 1.0 + filter_constant );
		}

		// Step through each marked movement trigger and verify that the velocity is 
		// close to zero when the trigger was sent.
		FindAnalysisEventRange( first, last );
		for ( i = first; i < last; i++ ) {
			if ( eventList[i].event == TRIGGER_MOVEMENT ) {
				index = TimeToFrame( eventList[i].time );
				for ( j = index; j > index - hold_frames && j > first; j-- ) {
					if ( tangential_velocity[i] > threshold ) {
						early_starts++;
						break;
					}
				}
			}
		}
		// Check if the computed number of cycles is in the desired range.
		error = ( early_starts > max_early_starts );

	}

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "To many false starts.";
		// The message could be different depending on whether the maniplulandum
		// was not moved enough, or if it just could not be seen.
		if ( N <= 0.0 ) fmt = "%s\n Manipulandum not visible.";
		else fmt = "%s\n False Starts Detected: %d\nMaximum Allowed: %d";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, early_starts, max_early_starts );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event.
	monitor->SendEvent( "Early Starts OK.\n Measured: %d\n Maximum Allowed: %d", early_starts, max_early_starts );
	return( NORMAL_EXIT );
	
}

/********************************************************************************************/

//
// Checks that the maniplandum is close to the specified target when the cue to start a movement occurs.
// Movement triggers must be marked by LogEvent( TRIGGER_MOVEMENT );
// 
// TO DO: Not yet tested!

int DexApparatus::CheckCorrectStartPosition(  int target_id, float tolX, float tolY, float tolZ, int max_bad_positions, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	int		bad_positions = 0, movements = 0;
	int		first, last;
	int		i, index;

	Vector3 delta;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Step through each marked movement trigger and verify that the velocity is 
	// close to zero when the trigger was sent.
	FindAnalysisEventRange( first, last );
	for ( i = first; i < last; i++ ) {
		if ( eventList[i].event == TRIGGER_MOVEMENT ) {
			movements++;
			index = TimeToFrame( eventList[i].time );
			SubtractVectors( delta, acquiredManipulandumState[index].position, targetPosition[target_id] );
			if ( fabs( delta[X] ) > tolX 
				 || fabs( delta[Y] ) > tolY 
				 || fabs( delta[Z] ) > tolZ ) bad_positions++;
		}
	}
	// Check if the computed number of incorrect starting positions is in the desired range.
	error = ( bad_positions > max_bad_positions );

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "Starting position not respected.";
		// The message could be different depending on whether the maniplulandum
		// was not moved enough, or if it just could not be seen.
		fmt = "%s\n  Total Movements: %d\n  Erroneous Positions: %d\nMaximum Allowed: %d";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, movements, bad_positions, max_bad_positions );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event.
	monitor->SendEvent( fmt, "Start positions OK.", movements, bad_positions, max_bad_positions );
	return( NORMAL_EXIT );
	
}
/********************************************************************************************/

//
// Checks that the subject starts off in the right direction.
// Movement triggers must be marked by LogEvent( TRIGGER_MOVE_UP and TRIGGER_MOVE_DOWN );
// 
// TO DO: Not fully tested yet!

int DexApparatus::CheckMovementDirection(  int max_false_directions, float dirX, float dirY, float dirZ, float threshold, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	int		bad_movements = 0, movements = 0, starts = 0;
	int		first, last;
	int		i, index;

	Vector3 dir;
	dir[X] = dirX;
	dir[Y] = dirY;
	dir[Z] = dirZ;
	float displacement;

	Vector3 start_position, delta;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Step through each marked upward movement trigger and count if 
	// the initial movement was in the right direction.
	FindAnalysisEventRange( first, last );
	for ( i = first; i < last; i++ ) {
		if ( eventList[i].event == TRIGGER_MOVE_UP ) {
			movements++;
			index = TimeToFrame( eventList[i].time );
			CopyVector( start_position, acquiredManipulandumState[index].position );
			while ( index < nAcqFrames ) {
				SubtractVectors( delta, acquiredManipulandumState[index].position, start_position );
				displacement = DotProduct( dir, delta );
				// 'UP' here means in the same direction and the specified vector.
				// If we move past the threshold in that direction before moving past the
				// same threshold distance in the other direction, then this movement is good.
				if ( displacement > threshold ) {
					starts++;
					break;
				}
				// If we go the threshold distance in the opposite direction first,
				// then consider this to have been an erroneous start.
				if ( displacement < - threshold ) {
					bad_movements++;
					starts++;
					break;
				}
				index++;
			}
		}
		else if ( eventList[i].event == TRIGGER_MOVE_DOWN ) {
			// Now do the same thing for downward movements.
				movements++;
			index = TimeToFrame( eventList[i].time );
			CopyVector( start_position, acquiredManipulandumState[index].position );
			while ( index < nAcqFrames ) {
				SubtractVectors( delta, acquiredManipulandumState[index].position, start_position );
				displacement = DotProduct( dir, delta );
				// Here, a negative movement is good ...
				if ( displacement < - threshold ) {
					starts++;
					break;
				}
				// ... and a positive movement is bad.
				if ( displacement > threshold ) {
					bad_movements++;
					starts++;
					break;
				}
				index++;
			}
		}
	}
	// Check if the computed number of incorrect starting positions is in the desired range.
	error = ( bad_movements > max_false_directions );

	// This format string is used to add debugging information to the event notification.
	// It is used whether there is an error or not.
	fmt = "%s\n  Total Movements: %d\n  Actual Starts: %d\n  Errors Detected: %d\n  Maximum Allowed: %d";

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "To many starts in wrong direction.";
		int response = fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, movements, starts, bad_movements, max_false_directions );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event to ground.
	monitor->SendEvent( fmt, "Start directions OK.", movements, starts, bad_movements, max_false_directions );
	return( NORMAL_EXIT );
	
}
/********************************************************************************************/

//
// Checks that the subject taps hard enough, but not too hard, with the manipulandum.
// Movement triggers must be marked by LogEvent( TRIGGER_MOVE_UP and TRIGGER_MOVE_DOWN );
// 
// TO DO: Not fully tested yet!

int DexApparatus::CheckForcePeaks( float min_amplitude, float max_amplitude, int max_bad_peaks, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	int		bad_peaks = 0, lows = 0, highs = 0; 
	int		movements = 0;
	int		first, last;
	int		start_sample, end_sample, smpl;
	int		i, j, n;

	double average, delta, peak;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Step through each marked movement trigger.
	FindAnalysisEventRange( first, last );
	for ( i = first; i < last - 1; i++ ) {
		if ( eventList[i].event == TRIGGER_MOVE_UP || eventList[i].event == TRIGGER_MOVE_DOWN ) {
			movements++;
			// Find when the next movement started.
			for ( j = i + 1; j < last - 1; j++ ) {
				if ( eventList[i].event == TRIGGER_MOVE_UP || eventList[i].event == TRIGGER_MOVE_DOWN ) break;
			}

			// Compute the average force through the movement.
			start_sample = TimeToSample( eventList[i].time );
			end_sample = TimeToSample( eventList[j].time );
			average = 0.0;
			n = 0;
			for ( smpl = start_sample; smpl < end_sample; smpl++ ) { 
				average += acquiredLoadForce[smpl][Y];
				n++;
			}
			average /= n;

			// Find the peak force relative to the average during the period.
			peak = 0.0;
			for ( smpl = start_sample; smpl < end_sample; smpl++ ) { 
				delta = fabs( acquiredLoadForce[smpl][Y] - average );
				if ( delta > peak ) peak = delta;
			}
			if ( peak < min_amplitude || peak > max_amplitude ) bad_peaks++;
			if ( peak < min_amplitude ) lows++;
			if ( peak > max_amplitude ) highs++;

		}
	}

	// Check if the computed number of incorrect starting positions is in the desired range.
	error = ( bad_peaks > max_bad_peaks );

	// This format string is used to add debugging information to the event notification.
	// It is used whether there is an error or not.
	fmt = "%s\n\n Min Force Peak: %.2f\n Max Force Peak: %.2f\n\nTotal Movements: %d\n  Errors Detected: %d\n   Highs: %d\n   Lows: %d\n  Maximum Allowed: %d";

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "To many collisions outside force range.";
		int response = 
			fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, min_amplitude, max_amplitude, 
							movements, bad_peaks, highs, lows, max_bad_peaks );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event to ground.
	monitor->SendEvent( fmt, "Peak Forces OK.", movements, bad_peaks, highs, lows, max_bad_peaks );
	return( NORMAL_EXIT );
	
}

/********************************************************************************************/

//
// Checks that the subject taps hard enough, but not too hard, with the manipulandum.
// Movement triggers must be marked by LogEvent( TRIGGER_MOVE_UP and TRIGGER_MOVE_DOWN );
// Here we use the accelerometer data instead of force data.
// TO DO: Not fully tested yet!

int DexApparatus::CheckAccelerationPeaks( float min_amplitude, float max_amplitude, int max_bad_peaks, const char *msg ) {
	
	const char *fmt;
	bool  error = false;

	int		bad_peaks = 0, lows = 0, highs = 0; 
	int		movements = 0;
	int		first, last;
	int		start_sample, end_sample, smpl;
	int		i, j, n;

	double average, delta, peak;

	// First we should look for the start and end of the actual movement based on 
	// events such as when the subject reaches the first target. 
	FindAnalysisFrameRange( first, last );

	// Step through each marked movement trigger.
	FindAnalysisEventRange( first, last );
	for ( i = first; i < last - 1; i++ ) {
		if ( eventList[i].event == TRIGGER_MOVE_UP || eventList[i].event == TRIGGER_MOVE_DOWN ) {
			movements++;
			// Find when the next movement started.
			for ( j = i + 1; j < last - 1; j++ ) {
				if ( eventList[i].event == TRIGGER_MOVE_UP || eventList[i].event == TRIGGER_MOVE_DOWN ) break;
			}

			// Compute the average force through the movement.
			start_sample = TimeToSample( eventList[i].time );
			end_sample = TimeToSample( eventList[j].time );
			average = 0.0;
			n = 0;
			for ( smpl = start_sample; smpl < end_sample; smpl++ ) { 
				average += acquiredHighAcceleration[smpl];
				n++;
			}
			average /= n;

			// Find the peak force relative to the average during the period.
			peak = 0.0;
			for ( smpl = start_sample; smpl < end_sample; smpl++ ) { 
				delta = fabs( acquiredHighAcceleration[smpl] - average );
				if ( delta > peak ) peak = delta;
			}
			if ( peak < min_amplitude || peak > max_amplitude ) bad_peaks++;
			if ( peak < min_amplitude ) lows++;
			if ( peak > max_amplitude ) highs++;

		}
	}

	// Check if the computed number of incorrect starting positions is in the desired range.
	error = ( bad_peaks > max_bad_peaks );

	// This format string is used to add debugging information to the event notification.
	// It is used whether there is an error or not.
	fmt = "%s\n\n Min Acc Peak: %.2f\n Max Acc Peak: %.2f\n\nTotal Movements: %d\n  Errors Detected: %d\n   Highs: %d\n   Lows: %d\n  Maximum Allowed: %d";

	// If not, signal the error to the subject.
	// Here I take the approach of calling a method to signal the error.
	// We agree instead that the routine should either return a null pointer
	// if there is no error, or return the error message as a static string.
	if ( error ) {
		
		// If the user provided a message to signal a visibilty error, use it.
		// If not, generate a generic message.
		if ( !msg ) msg = "To many collisions outside acceleration range.";
		int response = 
			fSignalError( MB_ABORTRETRYIGNORE, fmt, msg, min_amplitude, max_amplitude, 
							movements, bad_peaks, highs, lows, max_bad_peaks );
		
		if ( response == IDABORT ) return( ABORT_EXIT );
		if ( response == IDRETRY ) return( RETRY_EXIT );
		if ( response == IDIGNORE ) return( IGNORE_EXIT );
		
	}
	
	// This is my means of signalling the event to ground.
	monitor->SendEvent( fmt, "Peak Accelerations OK.", movements, bad_peaks, highs, lows, max_bad_peaks );
	return( NORMAL_EXIT );
	
}

/********************************************************************************************/

// Compute the strain guage offsets from the most recently acquired ADC data.
// Store them with the ATI calibration data to allow the offsets to be nullified.
// Because the acquired data is automatically sent to ground, we will be able
// to repeat this process on the ground during post-flight data processing.

void DexApparatus::ComputeAndNullifyStrainGaugeOffsets( void ) {

	double sum[N_GAUGES];
	float  gauge_offset[N_GAUGES];
	int smpl, unit, gge;

	// Take multiple samples and compute an average.
	// First zero the sums.
	for ( unit = 0; unit < nForceTransducers; unit++ ) {
		for ( gge = 0; gge < nGauges; gge++ ) {
			sum[gge] = 0.0;
		}

		// Accumulate the specified number of samples for the average.
		for ( smpl = 0; smpl < nAcqSamples; smpl++ ) {
			// Add the new samples to the sums.
			for ( gge = 0; gge < N_GAUGES; gge++ ) {
				sum[gge] += acquiredAnalog[smpl].channel[ ftAnalogChannel[unit] + gge ];
			}
		}
		for ( gge = 0; gge < N_GAUGES; gge++ ) {
			gauge_offset[gge] = sum[gge] / (double) nAcqSamples;
		}

		// It could be useful to send the bias values to the ground in real time.
		monitor->SendEvent( "Strain gauge offsets computed.\n Unit: %d < %f %f %f %f %f %f >", 
			unit, gauge_offset[0], gauge_offset[1], gauge_offset[2], gauge_offset[3], gauge_offset[4], gauge_offset[5] );

		NullifyStrainGaugeOffsets( unit, gauge_offset );

	} 
}
