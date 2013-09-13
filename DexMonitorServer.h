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

protected:

	// A pointer to the pipe where we send the messages.
	FILE			*fp;

	// Count the number of messages sent out;
	unsigned long		messageCounter;

public:

	DexMonitorServer( int vertical_targets = N_VERTICAL_TARGETS, 
					  int horizontal_targets = N_HORIZONTAL_TARGETS,
					  int codas = N_CODAS );
	void Quit( void );
	virtual void SendPacket( const char *packet ) = NULL;

	int SendState( bool acquisitionState, unsigned long targetState, 
									 bool manipulandum_visibility, 
									 Vector3 manipulandum_position, 
									 Quaternion manipulandum_orientation );

	int SendConfiguration( int nCodas, int nTargets, 
							DexSubjectPosture subjectPosture,
							DexTargetBarConfiguration targetBarConfig, 
							DexTappingSurfaceConfiguration tappingSurfaceConfig );

	void SendRecording( ManipulandumState state[], int samples, float availability_code );
	int SendEvent( const char* format, ... );	

};

class DexMonitorServerUDP : public DexMonitorServer {

private:

	// A data structure containing parameters for the UDP broadcast.
	DexUDP udp_parameters;

protected:

public:


	DexMonitorServerUDP( int vertical_targets = N_VERTICAL_TARGETS, 
					  int horizontal_targets = N_HORIZONTAL_TARGETS,
					  int codas = N_CODAS );
	void SendPacket( const char *packet );

};

class DexMonitorServerGUI : public DexMonitorServer {

private:

protected:

public:


	DexMonitorServerGUI( int vertical_targets = N_VERTICAL_TARGETS, 
					  int horizontal_targets = N_HORIZONTAL_TARGETS,
					  int codas = N_CODAS );
	void SendPacket( const char *packet );

};
