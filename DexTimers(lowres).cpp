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

#ifdef HIRES
	QueryPerformanceCounter( &timer.hr_mark );
	timer.hr_split = timer.hr_mark;
#else
	timer.mark = clock();
	timer.split = timer.mark;
#endif

}
	
/*****************************************************************************/

double DexTimerElapsedTime ( DexTimer &timer ) {
	
	clock_t		current_time;
	double		duration;
	
	/* Compute the true time interval since the timer was started. */

#ifdef HIRES
	QueryPerformanceCounter( &timer.hr_mark );
	timer.hr_split = timer.hr_mark;
#else
	current_time = clock();
	duration = (double) (current_time - timer.mark) / CLOCKS_PER_SEC / dex_slow_motion;

	return( duration );

}

double DexTimerRemainingTime( DexTimer &timer ) {

	return( timer.alarm - DexTimerElapsedTime( timer ) );

}

/****************************************************************************/

double DexTimerSplitTime ( DexTimer &timer ) {
	
	clock_t		current_time;
	double		duration;
	
  /* Compute the true time interval since the timer was started. */
	current_time = clock();
	duration = (double) (current_time - timer.split) / CLOCKS_PER_SEC / dex_slow_motion;
	timer.split = current_time;


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
