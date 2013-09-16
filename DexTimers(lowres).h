/*****************************************************************************/
/*                                                                           */
/*                                DexTimers.h                                */
/*                                                                           */
/*****************************************************************************/

/* Timers */

#ifndef _dex_timers_

#ifdef __cplusplus 
extern "C" {
#endif

#define HIRES
#include <time.h>
#ifdef HIRES
#include "Windows.h"
#endif

typedef struct {

#ifdef WIN32		/* Windows NT */

#ifdef HIRES

	LARGE_INTEGER hr_mark;
	LARGE_INTEGER hr_split;

#else

    clock_t	mark;	
    clock_t	split;

#endif

    double	alarm;

#else				/* SGI Irix */

    timespec_t	mark;
    timespec_t	split;
    double		alarm;

#endif

 

} DexTimer;

void	DexTimerStart ( DexTimer &timer );
double	DexTimerElapsedTime ( DexTimer &timer );
double	DexTimerRemainingTime( DexTimer &timer );
double	DexTimerSplitTime ( DexTimer &timer );
void	DexTimerSet( DexTimer &timer, double seconds );
int	    DexTimerTimeout( DexTimer &timer );
void	DexTimerSetSlowmotion( float factor );

#ifdef __cplusplus 
}
#endif

#define _dex_timers_

#endif
