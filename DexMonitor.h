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

class DexMonitor {

private:

	// 2D Graphics
	Display display; 
	Layout  layout;
	View	view, xz_view;

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

	float recordedTime[DEX_MAX_MARKER_FRAMES];
	float recordedPosition[DEX_MAX_MARKER_FRAMES * 3];
	float recordedOrientation[DEX_MAX_MARKER_FRAMES * 3];
	int	  nAcqFrames;

	void PlotAcquisition( float *positions, float *orientations, int frames, float availability_flag );

	int  Update( void );
	void RunWindow( void );
	void ParseInputStream( FILE *fp );
	void ParseInputPacket( char *packet );

	
};



#endif