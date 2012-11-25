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

#include <time.h>


typedef struct {

#ifdef WIN32		/* Windows NT */

    clock_t	mark;	
    clock_t	split;
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
