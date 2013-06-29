/*********************************************************************************/
/*                                                                               */
/*                             DexTargetCalibration.cpp                          */
/*                                                                               */
/*********************************************************************************/

/*
 * This not a task that one might expect a astronaut to perform.
 * Rather, it is a convenient way to store the 3D location of each target.
 * This is convenient for the simulator, where target structures and trackers can change.
 * I don't know how Qinetiq manages this issue.
 */

/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include <VectorsMixin.h>
#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

/*********************************************************************************/

int RunTargetCalibration( DexApparatus *apparatus, const char *params ) {
	int exit_status;
	exit_status = apparatus->CalibrateTargets();
	return( exit_status );
}



