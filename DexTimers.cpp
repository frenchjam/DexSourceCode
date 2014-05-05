/*****************************************************************************/
/*                                                                           */
/*                               DexTimers.cpp                               */
/*                                                                           */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "DexTimers.h"

// 
// Scale factor to produce slow motion.
// Set it to 1.0 to produce normal speed movements.
// Note that all the timers share this common factor.
//

static double dex_slow_motion = 1.0;

/*****************************************************************************/

void DexTimerSetSlowmotion( float factor ) {
	dex_slow_motion = factor;
}


/*****************************************************************************/

/*
 * Implement a means of timing actions - version Win32.
 */

/****************************************************************************/

/*
 * Simply start the timer without setting an alarm.
 * Used to measure elapsed time rather than to wait for a timeout.
 */

void DexTimerStart ( DexTimer &timer ) {

	LARGE_INTEGER	li;

	QueryPerformanceFrequency( &li );
	timer.frequency = (double) li.QuadPart;

	QueryPerformanceCounter( &li );
	timer.mark = li.QuadPart;

	timer.split = timer.mark;

}
	
/*****************************************************************************/

double DexTimerElapsedTime ( DexTimer &timer ) {
	
	LARGE_INTEGER	li;
	__int64			current_time;
	double			duration;
	
	/* Compute the true time interval since the timer was started. */

	QueryPerformanceCounter( &li );
	current_time = li.QuadPart;
	duration = (double) (current_time - timer.mark) / timer.frequency / dex_slow_motion;

	return( duration );

}

double DexTimerRemainingTime( DexTimer &timer ) {
	return( timer.alarm - DexTimerElapsedTime( timer ) );
}

/****************************************************************************/

double DexTimerSplitTime ( DexTimer &timer ) {
	
	LARGE_INTEGER	li;
	__int64			current_time;
	double			duration;
	
	/* Compute the true time interval since the last split was set. */

	QueryPerformanceCounter( &li );
	current_time = li.QuadPart;
	duration = (double) (current_time - timer.split) / timer.frequency / dex_slow_motion;

	return( duration );

}

/****************************************************************************/

void DexTimerSet( DexTimer &timer, double seconds ) {

	timer.alarm = seconds;
	DexTimerStart( timer );

}

int	DexTimerTimeout( DexTimer &timer ) {
	return( DexTimerElapsedTime( timer ) >= timer.alarm );
}
