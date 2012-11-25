/*********************************************************************************/
/*                                                                               */
/*                                  DexMonitor.h                                 */
/*                                                                               */
/*********************************************************************************/

/*
 * Graphical monitoring of the DEX hardware.
 */

#ifndef DexMonitorH
#define DexMonitorH
#include <OpenGLObjects.h>
#include <OpenGLUseful.h>
#include <OpenGLColors.h>

#include "Dexterous.h"

#include <Views.h>
#include <Layouts.h>


class DexMonitor {

private:

	// 2D Graphics
	Display display; 
	Layout  layout;
	View		view, xy_view;

	// OpenGL objects representing the CODA bars and target frame.	
	OpenGLWindow				*window;
	AfdEnvironment				*room;
	CODA						*coda[DEX_MAX_CODAS];					
	DexVirtualTargetBar			*targets;
	Slab						*manipulandum;

	int nTargets;
	int nCodas;

	bool missed_frames;

	float targetPosition[DEX_MAX_TARGETS][3];

	void Draw( void );
	void Plot( void );

	DexUDP udp_parameters;


protected:

public:


	DexMonitor( int screen_left = 0, int screen_top = 0,
		int n_vertical_targets = N_VERTICAL_TARGETS, 
		int n_horizontal_targets = N_HORIZONTAL_TARGETS,
		int codas = N_CODAS );

	void Quit( void );

	void StartAcquisition( void );
	void StopAcquisition( void );
	void SetTargetConfiguration( int configID );
	void TargetOn( int n );
	void TargetsOff( void );
	void Beep( void );
	void Boop( void );

	void SetManipulandumPosition( float pos[3] );

	float recordedTime[DEX_MAX_DATA_FRAMES];
	float recordedPosition[DEX_MAX_DATA_FRAMES * 3];
	float recordedOrientation[DEX_MAX_DATA_FRAMES * 3];
	int	  nAcqFrames;

	void PlotAcquisition( float *positions, float *orientations, int frames, float availability_flag );

	int  Update( void );
	void RunWindow( void );
	void ParseInputStream( FILE *fp );
	void ParseInputPacket( char *packet );

	
};

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

#endif