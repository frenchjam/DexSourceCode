/*********************************************************************************/
/*                                                                               */
/*                               DexMonitorServer.h                              */
/*                                                                               */
/*********************************************************************************/

#pragma once

#include <DexUDPServices.h>
#include <Dexterous.h>

class DexMonitorServer {

private:

	// Count the number of messages sent out;
	unsigned long		messageCounter;
	// A pointer to the pipe where we send the messages.
	FILE			*fp;
	// A data structure containing parameters for the UDP broadcast.
	DexUDP udp_parameters;

protected:

public:


	DexMonitorServer( int vertical_targets = N_VERTICAL_TARGETS, 
						int horizontal_targets = N_HORIZONTAL_TARGETS,
						int codas = N_CODAS );
	void Quit( void );

	int DexMonitorServer::SendState( bool acquisitionState, unsigned long targetState, 
									 bool manipulandum_visibility, float manipulandum_position[3], 
									 float manipulandum_orientation[4] );

	int SendConfiguration( int nCodas, int nTargets, 
							DexSubjectPosture subjectPosture,
							DexTargetBarConfiguration targetBarConfig, 
							DexTappingSurfaceConfiguration tappingSurfaceConfig );

	void DexMonitorServer::SendRecording( ManipulandumState state[], int samples, float availability_code );
	int SendEvent( const char* format, ... );	

};
