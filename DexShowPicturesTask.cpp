/*********************************************************************************/
/*                                                                               */
/*                          DexCancelOffsetsTask.cpp                             */
/*                                                                               */
/*********************************************************************************/

/*
 * Performs the operations to measure and nullify the offets on the ATI strain gauges.
 */

/*********************************************************************************/

#include <windows.h>
#include <mmsystem.h>

#include <stdio.h>
#include <stdlib.h>

#include "DexApparatus.h"
#include "Dexterous.h"
#include "DexTasks.h"

char *pictures[] = { "OK.bmp", "HandsOff.bmp", "coef_frict_osc.bmp",  NULL };

/*********************************************************************************/

int ShowPictures( DexApparatus *apparatus, const char *params ) {

	char **pic;

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );
	apparatus->Comment( "Puts up pictures. Just used for debugging." );

	for ( pic = pictures; *pic; pic++ ) {

		apparatus->fShowStatus( *pic, "File: %s\nAs STATUS.", *pic );
		apparatus->Wait( 3.0 );
		apparatus->fWaitSubjectReady( *pic, "File: %s\nAs POPUP MENU.", *pic );

	}

	// Clearly demark this operation in the script file. 
	apparatus->Comment( "################################################################################" );
 
	return( NORMAL_EXIT );

}


